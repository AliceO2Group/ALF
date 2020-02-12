// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file Swt.cxx
/// \brief Definition of SWT operations
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <boost/format.hpp>
#include <chrono>
#include <thread>

#include "AlfException.h"
#include "Logger.h"
#include "ReadoutCard/Cru.h"
#include "Swt/Swt.h"

namespace AliceO2
{
namespace Alf
{

namespace sc_regs = AliceO2::roc::Cru::ScRegisters;

Swt::Swt(AlfLink link) : mBar2(*link.bar2), mLink(link)
{
  setChannel(mLink.linkId);
}

void Swt::setChannel(int gbtChannel)
{
  barWrite(sc_regs::SC_LINK.index, gbtChannel);
}

void Swt::reset()
{
  barWrite(sc_regs::SC_RESET.index, 0x1);
  barWrite(sc_regs::SC_RESET.index, 0x0); //void cmd to sync clocks
  mWordSequence = 0;
}

void Swt::read(std::vector<SwtWord>& words, TimeOut msTimeOut, SwtWord::Size wordSize)
{
  uint32_t numWords = 0x0;

  auto timeOut = std::chrono::steady_clock::now() + std::chrono::milliseconds(msTimeOut);
  while ((std::chrono::steady_clock::now() < timeOut) && (numWords < 1)) {
    numWords = (barRead(sc_regs::SWT_MON.index) >> 16);
  }

  if (numWords < 1) { // #WORDS in READ FIFO
    BOOST_THROW_EXCEPTION(SwtException() << ErrorInfo::Message("Not enough words in SWT READ FIFO"));
  }

  for (int i = 0; i < (int)numWords; i++) {
    SwtWord tempWord;

    barWrite(sc_regs::SWT_CMD.index, 0x2);
    barWrite(sc_regs::SWT_CMD.index, 0x0); // void cmd to sync clocks

    tempWord.setLow(barRead(sc_regs::SWT_RD_WORD_L.index));
    if (wordSize == SwtWord::Size::Low) {
      continue;
    }
    tempWord.setMed(barRead(sc_regs::SWT_RD_WORD_M.index));
    if (wordSize == SwtWord::Size::Medium) {
      continue;
    }
    tempWord.setHigh(barRead(sc_regs::SWT_RD_WORD_H.index));

    mWordSequence = (mWordSequence + 1) % 16;

    // If we get the same counter as before it means the FIFO wasn't updated; drop the word
    if (tempWord.getSequence() != mWordSequence) {

      if (std::chrono::steady_clock::now() < timeOut) {
        BOOST_THROW_EXCEPTION(SwtException() << ErrorInfo::Message("Timed out: not enough words in SWT READ FIFO"));
      }

      getWarningLogger() << "SWT word sequence duplicate" << endm;

      mWordSequence = tempWord.getSequence(); //roll mWordSequence back by one
      numWords++;                             //will have to read one more time
      continue;
    }

    //wordMonPairs.push_back(std::make_pair(tempWord, barRead(sc_regs::SWT_MON.index)));
    words.push_back(tempWord);
  }
}

uint32_t Swt::write(const SwtWord& swtWord)
{
  // prep the swt word
  if (swtWord.getHigh()) {
    barWrite(sc_regs::SWT_WR_WORD_H.index, swtWord.getHigh());
  }
  if (swtWord.getMed() || swtWord.getHigh()) {
    barWrite(sc_regs::SWT_WR_WORD_M.index, swtWord.getMed());
  }
  barWrite(sc_regs::SWT_WR_WORD_L.index, swtWord.getLow()); // The LOW bar write, triggers the write operation

  return barRead(sc_regs::SWT_MON.index);
}

void Swt::barWrite(uint32_t index, uint32_t data)
{
  mBar2.writeRegister(index, data);
}

uint32_t Swt::barRead(uint32_t index)
{
  uint32_t read = mBar2.readRegister(index);
  return read;
}

std::string Swt::writeSequence(std::vector<std::pair<SwtData, Operation>> sequence)
{
  std::stringstream resultBuffer;
  for (const auto& it : sequence) {
    SwtData data = it.first;
    try {
      if (it.second == Operation::Read) {
        std::vector<SwtWord> results;

        int timeOut;
        try {
          timeOut = std::get<TimeOut>(data);
        } catch (...) { // no timeout was provided
          data = DEFAULT_SWT_TIMEOUT_MS;
          timeOut = std::get<TimeOut>(data);
        }
        read(results, timeOut);

        for (const auto& result : results) {
          resultBuffer << result << "\n";
        }
      } else if (it.second == Operation::Write) {
        write(std::get<SwtWord>(data));
        resultBuffer << "0"
                     << "\n";
      } else if (it.second == Operation::Reset) {
        reset();
      } else {
        BOOST_THROW_EXCEPTION(SwtException() << ErrorInfo::Message("SWT operation type unknown"));
      }
    } catch (const SwtException& e) {
      // If an SWT error occurs, we stop executing the sequence of commands and return the results as far as we got them, plus
      // the error message.
      std::string meaningfulMessage;
      if (it.second == Operation::Read) {
        meaningfulMessage = (boost::format("SWT_SEQUENCE READ timeout=%d serial=%d link=%d, error='%s'") % std::get<TimeOut>(data) % mLink.serial % mLink.linkId % e.what()).str();
      } else if (it.second == Operation::Write) {
        meaningfulMessage = (boost::format("SWT_SEQUENCE WRITE data=%s serial=%d link=%d, error='%s'") % std::get<SwtWord>(data) % mLink.serial % mLink.linkId % e.what()).str();
      } else {
        meaningfulMessage = (boost::format("SWT_SEQUENCE RESET serial=%d link=%d, error='%s'") % mLink.serial % mLink.linkId % e.what()).str();
      }
      getErrorLogger() << meaningfulMessage << endm;
      resultBuffer << meaningfulMessage;
      BOOST_THROW_EXCEPTION(SwtException() << ErrorInfo::Message(resultBuffer.str()));
    }
  }
  return resultBuffer.str();
}

} // namespace Alf
} // namespace AliceO2
