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

#include "boost/format.hpp"
#include <chrono>
#include <thread>

#include "AlfException.h"
#include "Logger.h"
#include "Swt/Swt.h"

#include "ReadoutCard/Cru.h"

namespace AliceO2
{
namespace Alf
{

namespace sc_regs = AliceO2::roc::Cru::ScRegisters;

Swt::Swt(roc::RegisterReadWriteInterface &bar2, AlfLink link) : mBar2(bar2), mLink(link)
{
  reset();
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
}

void Swt::read(std::vector<std::pair<SwtWord, uint32_t>> &wordMonPairs, SwtWord::Size wordSize)
{
  uint32_t numWords = 0x0;

  auto endTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(10);
  while (std::chrono::steady_clock::now() <= endTime) {
    numWords = barRead(sc_regs::SWT_MON.index);
  }

  if ((numWords >> 16) < 1) { // #WORDS in READ FIFO
    BOOST_THROW_EXCEPTION(SwtException() << ErrorInfo::Message("Exceeded timeout on busy wait!")); //TODO: Don't crash
  }

  for (int i = 0; i < (int) numWords; i++) {
    SwtWord tempWord;

    barWrite(sc_regs::SWT_CMD.index, 0x2);
    barWrite(sc_regs::SWT_CMD.index, 0x0); // void cmd to sync clocks

    tempWord.setLow(barRead(sc_regs::SWT_RD_WORD_L.index));
    if (wordSize == SwtWord::Size::Low) { continue; }
    tempWord.setMed(barRead(sc_regs::SWT_RD_WORD_M.index));
    if (wordSize == SwtWord::Size::Medium) { continue; }
    tempWord.setHigh(barRead(sc_regs::SWT_RD_WORD_H.index));

    wordMonPairs.push_back(std::make_pair(tempWord, barRead(sc_regs::SWT_MON.index)));
  }
}

uint32_t Swt::write(const SwtWord& swtWord)
{
  // prep the swt word
  barWrite(sc_regs::SWT_WR_WORD_L.index, swtWord.getLow());
  if (swtWord.getMed()) { 
    barWrite(sc_regs::SWT_WR_WORD_M.index, swtWord.getMed());
  }
  if (!swtWord.getHigh()) {
    barWrite(sc_regs::SWT_WR_WORD_H.index, swtWord.getHigh());
  }

  // perform write
  barWrite(sc_regs::SWT_CMD.index, 0x1);
  barWrite(sc_regs::SWT_CMD.index, 0x0); //void cmd to sync clocks

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

std::string Swt::writeSequence(std::vector<SwtWord> words)
{
  std::stringstream resultBuffer;
  for (const auto& word : words) {
    try {
      write(word);
      if (word.getHigh() & 0x80000000) {
        //getLogger() << "data=" << word << endm;
        resultBuffer << "0" << "\n";
      } else {
        std::vector<std::pair<SwtWord, uint32_t>> results;
        read(results);
        for (const auto& element : results) {
          resultBuffer << element.first;
        }
      }
    } catch (const SwtException &e) {
      // If an SWT error occurs, we stop executing the sequence of commands and return the results as far as we got them, plus
      // the error message. //TODO: Rework this, it doesn't look right (same for SCA)
      getErrorLogger() << AliceO2::InfoLogger::InfoLogger::InfoLogger::Error << "SWT_SEQUENCE data=" << word << (boost::format("serial=%d link=%d, error='%s'") % mLink.serial % mLink.linkId % e.what()).str() << endm;
      resultBuffer << e.what();
      break;
    }
  }
  return resultBuffer.str();
}

} // namespace Alf
} // namespace AliceO2
