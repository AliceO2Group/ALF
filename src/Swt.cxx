
// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
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

#include "Alf/Exception.h"
#include "Logger.h"
#include "ReadoutCard/CardDescriptor.h"
#include "ReadoutCard/CardFinder.h"
#include "ReadoutCard/ChannelFactory.h"
#include "ReadoutCard/Cru.h"
#include "Alf/Swt.h"

namespace o2
{
namespace alf
{

namespace sc_regs = AliceO2::roc::Cru::ScRegisters;

Swt::Swt(AlfLink link, std::shared_ptr<lla::Session> llaSession, SwtWord::Size swtWordSize)
  : ScBase(link, llaSession), mSwtWordSize(swtWordSize)
{
  if (kDebugLogging) {
    Logger::setFacility("ALF/SWT");
  }
}

Swt::Swt(const roc::Parameters::CardIdType& cardId, int linkId)
  : ScBase(cardId, linkId)
{
  if (kDebugLogging) {
    Logger::setFacility("ALF/SWT");
  }
}

Swt::Swt(std::string cardId, int linkId)
  : ScBase(cardId, linkId)
{
  if (kDebugLogging) {
    Logger::setFacility("ALF/SWT");
  }
}

std::vector<SwtWord> Swt::read(SwtWord::Size wordSize, TimeOut msTimeOut)
{
  checkChannelSet();

  std::vector<SwtWord> words;
  uint32_t numWords = 0x0;

  auto timeOut = std::chrono::steady_clock::now() + std::chrono::milliseconds(msTimeOut);
  while ((std::chrono::steady_clock::now() < timeOut) && numWords < 1) {
    numWords = (barRead(sc_regs::SWT_MON.index) >> 16);
  }

  if (numWords < 1) { // #WORDS in READ FIFO
    BOOST_THROW_EXCEPTION(SwtException() << ErrorInfo::Message("Not enough words in SWT READ FIFO"));
  }

  for (int i = 0; i < (int)numWords; i++) {
    SwtWord tempWord;

    tempWord.setLow(barRead(sc_regs::SWT_RD_WORD_L.index));

    if (wordSize == SwtWord::Size::Medium || wordSize == SwtWord::Size::High) {
      tempWord.setMed(barRead(sc_regs::SWT_RD_WORD_M.index));
    }

    if (wordSize == SwtWord::Size::High) {
      tempWord.setHigh(barRead(sc_regs::SWT_RD_WORD_H.index));
    }

    //wordMonPairs.push_back(std::make_pair(tempWord, barRead(sc_regs::SWT_MON.index)));
    words.push_back(tempWord);
  }

  return words;
}

void Swt::write(const SwtWord& swtWord)
{
  checkChannelSet();

  // prep the swt word
  if (swtWord.getSize() == SwtWord::Size::High) {
    barWrite(sc_regs::SWT_WR_WORD_H.index, swtWord.getHigh());
  }
  if (swtWord.getSize() == SwtWord::Size::High || swtWord.getSize() == SwtWord::Size::Medium) {
    barWrite(sc_regs::SWT_WR_WORD_M.index, swtWord.getMed());
  }
  barWrite(sc_regs::SWT_WR_WORD_L.index, swtWord.getLow()); // The LOW bar write, triggers the write operation

  //return barRead(sc_regs::SWT_MON.index);
}

std::vector<std::pair<Swt::Operation, Swt::Data>> Swt::executeSequence(std::vector<std::pair<Operation, Data>> sequence, bool lock)
{
  if (lock) {
    mLlaSession->start();
  }

  try {
    checkChannelSet();
  } catch (const SwtException& e) {
    return { { Operation::Error, e.what() } };
  }

  std::vector<std::pair<Operation, Data>> ret;

  for (const auto& it : sequence) {
    Operation operation = it.first;
    Data data = it.second;
    try {
      if (operation == Operation::Read) {
        int timeOut;
        try {
          timeOut = boost::get<TimeOut>(data);
        } catch (...) { // no timeout was provided
          data = DEFAULT_SWT_TIMEOUT_MS;
          timeOut = boost::get<TimeOut>(data);
        }
        auto results = read(mSwtWordSize, timeOut);

        for (const auto& result : results) {
          ret.push_back({ Operation::Read, result });
        }
      } else if (operation == Operation::Write) {
        SwtWord word = boost::get<SwtWord>(data);
        write(word);
        ret.push_back({ Operation::Write, word });
        //ret.push_back({ Operation::Write, {} }); // TODO: Is it better to return {} ?
      } else if (operation == Operation::SCReset) {
        scReset();
        ret.push_back({ Operation::SCReset, {} });
      } else if (operation == Operation::Wait) {
        int waitTime;
        try {
          waitTime = boost::get<WaitTime>(data);
        } catch (...) { // no timeout was provided
          data = DEFAULT_SWT_WAIT_TIME_MS;
          waitTime = boost::get<WaitTime>(data);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(waitTime));
        ret.push_back({ operation, waitTime });
      } else {
        BOOST_THROW_EXCEPTION(SwtException() << ErrorInfo::Message("SWT operation type unknown"));
      }
    } catch (const SwtException& e) {
      std::string meaningfulMessage;
      if (operation == Operation::Read) {
        meaningfulMessage = (boost::format("SWT_SEQUENCE READ timeout=%d serialId=%s link=%d, error='%s'") % boost::get<TimeOut>(data) % mLink.serialId % mLink.linkId % e.what()).str();
      } else if (operation == Operation::Write) {
        meaningfulMessage = (boost::format("SWT_SEQUENCE WRITE data=%s serialId=%s link=%d, error='%s'") % boost::get<SwtWord>(data) % mLink.serialId % mLink.linkId % e.what()).str();
      } else if (operation == Operation::SCReset) {
        meaningfulMessage = (boost::format("SWT_SEQUENCE SC RESET serialId=%d link=%s, error='%s'") % mLink.serialId % mLink.linkId % e.what()).str();
      } else if (operation == Operation::Wait) {
        meaningfulMessage = (boost::format("SWT_SEQUENCE WAIT waitTime=%d serialId=%s link=%d error='%s'") % boost::get<WaitTime>(data) % mLink.serialId % mLink.linkId % e.what()).str();
      } else {
        meaningfulMessage = (boost::format("SWT_SEQUENCE UNKNOWN serialId=%d link=%s,  error='%s'") % mLink.serialId % mLink.linkId % e.what()).str();
      }
      //Logger::get().err() << meaningfulMessage << endm;

      ret.push_back({ Operation::Error, meaningfulMessage });
      break;
    }
  }

  if (lock) {
    mLlaSession->stop();
  }

  return ret;
}

std::string Swt::writeSequence(std::vector<std::pair<Operation, Data>> sequence, bool lock)
{
  std::stringstream resultBuffer;
  auto out = executeSequence(sequence, lock);
  for (const auto& it : out) {
    Operation operation = it.first;
    Data data = it.second;
    if (operation == Operation::Read) {
      resultBuffer << data << "\n";
    } else if (operation == Operation::Write) {
      resultBuffer << "0\n";
    } else if (operation == Operation::SCReset) {
      /* DO NOTHING */
    } else if (operation == Operation::Wait) {
      resultBuffer << std::dec << data << "\n";
    } else if (operation == Operation::Error) {
      resultBuffer << data;
      if (kDebugLogging) {
        Logger::get() << data << LogErrorDevel << endm;
      }
      BOOST_THROW_EXCEPTION(SwtException() << ErrorInfo::Message(resultBuffer.str()));
      break;
    }
  }

  return resultBuffer.str();
}

std::string Swt::SwtOperationToString(Swt::Operation op)
{
  if (op == Swt::Operation::Read) {
    return "read";
  } else if (op == Swt::Operation::Write) {
    return "write";
  } else if (op == Swt::Operation::SCReset) {
    return "sc_reset";
  } else if (op == Swt::Operation::Wait) {
    return "wait";
  } else if (op == Swt::Operation::Lock) {
    return "lock";
  } else if (op == Swt::Operation::Error) {
    return "error";
  }

  BOOST_THROW_EXCEPTION(SwtException() << ErrorInfo::Message("Cannot convert SWT operation to string"));
}

Swt::Operation Swt::StringToSwtOperation(std::string op)
{
  if (op == "read") {
    return Swt::Operation::Read;
  } else if (op == "write") {
    return Swt::Operation::Write;
  } else if (op == "sc_reset") {
    return Swt::Operation::SCReset;
  } else if (op == "wait") {
    return Swt::Operation::Wait;
  } else if (op == "lock") {
    return Swt::Operation::Lock;
  } else if (op == "error") {
    return Swt::Operation::Error;
  }

  BOOST_THROW_EXCEPTION(SwtException() << ErrorInfo::Message("Cannot convert operation to SWT string " + op));
}

} // namespace alf
} // namespace o2
