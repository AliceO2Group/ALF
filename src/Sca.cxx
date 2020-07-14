// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file Sca.h
/// \brief Implementation of ALICE Lowlevel Frontend (ALF) SCA operations
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <boost/format.hpp>
#include <chrono>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>

#include "ReadoutCard/CardFinder.h"
#include "ReadoutCard/ChannelFactory.h"
#include "ReadoutCard/Cru.h"

#include "Alf/Exception.h"
#include "Alf/Sca.h"

#include "Logger.h"
#include "Util.h"

namespace sc_regs = AliceO2::roc::Cru::ScRegisters;

namespace o2
{
namespace alf
{

Sca::Sca(AlfLink link)
  : mBar2(link.bar), mLink(link)
{
  if (mLink.linkId >= CRU_NUM_LINKS) {
    BOOST_THROW_EXCEPTION(
      ScaException() << ErrorInfo::Message("Maximum link number exceeded"));
  }

  barWrite(sc_regs::SC_RESET.index, 0x1);
  barWrite(sc_regs::SC_RESET.index, 0x0);

  mLlaSession = std::make_unique<LlaSession>("DDT", link.cardSequence);
}

Sca::Sca(const roc::Parameters::CardIdType& cardId, int linkId)
{
  init(cardId, linkId);
}

Sca::Sca(std::string cardId, int linkId)
{
  init(roc::Parameters::cardIdFromString(cardId), linkId);
}

void Sca::init(const roc::Parameters::CardIdType& cardId, int linkId)
{
  auto card = roc::findCard(cardId); //TODO: Use it elsewhere??

  mBar2 = roc::ChannelFactory().getBar(cardId, 2);
  mLink = AlfLink{
    "DDT",
    card.sequenceId,
    linkId,
    mBar2,
    roc::CardType::Cru
  };

  barWrite(sc_regs::SC_RESET.index, 0x1);
  barWrite(sc_regs::SC_RESET.index, 0x0);

  mLlaSession = std::make_unique<LlaSession>("DDT", card.sequenceId);
}

void Sca::setChannel(int gbtChannel)
{
  mLink.linkId = gbtChannel;
  barWrite(sc_regs::SC_LINK.index, mLink.linkId);
}

void Sca::checkChannelSet()
{
  if (mLink.linkId == -1) {
    BOOST_THROW_EXCEPTION(ScaException() << ErrorInfo::Message("No SCA channel selected"));
  }

  uint32_t channel = (barRead(sc_regs::SWT_MON.index) >> 8) & 0xff;

  if (channel != mLink.linkId) {
    setChannel(mLink.linkId);
  }
}

Sca::CommandData Sca::executeCommand(uint32_t command, uint32_t data, bool lock)
{
  if (lock) {
    mLlaSession->start();
  }

  checkChannelSet();

  CommandData result;
  try {
    write(command, data);
    result = read();
  } catch (const ScaException& e) {
    if (lock) {
      mLlaSession->stop();
    }
    BOOST_THROW_EXCEPTION(e);
  }

  if (lock) {
    mLlaSession->stop();
  }

  return result;
}

void Sca::write(uint32_t command, uint32_t data)
{
  waitOnBusyClear();
  barWrite(sc_regs::SCA_WR_DATA.index, data);
  barWrite(sc_regs::SCA_WR_CMD.index, command);
  auto transactionId = (command >> 16) & 0xff;
  if (transactionId == 0x0 || transactionId == 0xff) {
    BOOST_THROW_EXCEPTION(ScaException()
                          << ErrorInfo::Message("Invalid transaction ID"));
  }

  try {
    execute();
  } catch (const ScaException& e) {
    throw;
  }
}

Sca::CommandData Sca::read()
{
  waitOnBusyClear();
  auto data = barRead(sc_regs::SCA_RD_DATA.index);
  auto command = barRead(sc_regs::SCA_RD_CMD.index);
  /* printf("Sca::read   DATA=0x%x   CH=0x%x   TR=0x%x   CMD=0x%x\n", data,
   command >> 24, (command >> 16) & 0xff, command & 0xff);*/

  auto endTime = std::chrono::steady_clock::now() + CHANNEL_BUSY_TIMEOUT;
  while (std::chrono::steady_clock::now() < endTime) {
    if (!isChannelBusy(command)) {
      checkError(command);
      return { command, data };
    }
    data = barRead(sc_regs::SCA_RD_DATA.index);
    command = barRead(sc_regs::SCA_RD_CMD.index);
  }

  std::stringstream ss;
  ss << "command: " << command << " data: " << data;
  BOOST_THROW_EXCEPTION(ScaException() << ErrorInfo::Message(
                          "Exceeded timeout on channel busy wait" + ss.str()));
}

bool Sca::isChannelBusy(uint32_t command)
{
  return (command & 0xff) == 0x40;
}

void Sca::checkError(uint32_t command)
{
  uint32_t errorCode = command & 0xff;

  auto toString = [&](int flag) {
    switch (flag) {
      case 1:
        return "invalid channel request";
      case 2:
        return "invalid command request";
      case 3:
        return "invalid transaction number";
      case 4:
        return "invalid length";
      case 5:
        return "channel not enabled";
      case 6:
        return "channel busy";
      case 7:
        return "channel busy";
      case 0:
      default:
        return "generic error flag";
    }
  };

  // Check which error bits are enabled
  std::vector<int> flags;
  for (int flag = 0; flag < 7; ++flag) {
    if (Util::getBit(errorCode, flag) == 1) {
      flags.push_back(flag);
    }
  }

  // Turn into an error message
  if (!flags.empty()) {
    std::stringstream stream;
    stream << "error code 0x" << errorCode << ": ";
    for (size_t i = 0; i < flags.size(); ++i) {
      stream << toString(flags[i]);
      if (i < flags.size()) {
        stream << ", ";
      }
    }

    BOOST_THROW_EXCEPTION(ScaException() << ErrorInfo::Message(
                            stream.str()));
  }
}

void Sca::barWrite(uint32_t index, uint32_t data)
{
  mBar2->writeRegister(index, data);
}

uint32_t Sca::barRead(uint32_t index)
{
  return mBar2->readRegister(index);
}

void Sca::execute()
{
  barWrite(sc_regs::SCA_WR_CTRL.index, 0x4);
  barWrite(sc_regs::SCA_WR_CTRL.index, 0x0);
  waitOnBusyClear();
}

void Sca::waitOnBusyClear()
{
  auto endTime = std::chrono::steady_clock::now() + BUSY_TIMEOUT;
  while (std::chrono::steady_clock::now() < endTime) {
    if ((((barRead(sc_regs::SCA_RD_CTRL.index)) >> 31) & 0x1) == 0) {
      return;
    }
  }

  BOOST_THROW_EXCEPTION(ScaException()
                        << ErrorInfo::Message("Exceeded timeout on busy wait"));
}

std::vector<std::pair<Sca::Operation, Sca::Data>> Sca::executeSequence(const std::vector<std::pair<Operation, Data>>& operations, bool lock)
{
  if (lock) {
    mLlaSession->start();
  }

  try {
    checkChannelSet();
  } catch (const ScaException& e) {
    return { { Operation::Error, e.what() } };
  }

  std::vector<std::pair<Sca::Operation, Sca::Data>> ret;
  for (const auto& it : operations) {
    Operation operation = it.first;
    Data data = it.second;
    try {
      if (operation == Operation::Command) {
        auto commandData = boost::get<CommandData>(data);
        auto result = executeCommand(commandData);
        ret.push_back({ operation, result });
      } else if (operation == Operation::Wait) {
        int waitTime;
        try {
          waitTime = boost::get<WaitTime>(data);
        } catch (...) { // no timeout provided
          data = DEFAULT_SCA_WAIT_TIME_MS;
          waitTime = boost::get<WaitTime>(data);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(waitTime));
        ret.push_back({ operation, waitTime });
      } else {
        BOOST_THROW_EXCEPTION(ScaException() << ErrorInfo::Message("SCA operation type unknown"));
      }
    } catch (const ScaException& e) {
      // If an SCA error occurs, we stop executing the sequence of commands and return the results as far as we got
      // them, plus the error message.
      std::string meaningfulMessage;
      if (operation == Operation::Command) {
        meaningfulMessage = (boost::format("SCA_SEQUENCE cmd=0x%08x data=0x%08x cardSequence=%d link=%d error='%s'") % boost::get<CommandData>(data).command % boost::get<CommandData>(data).data % mLink.cardSequence % mLink.linkId % e.what()).str();
      } else if (operation == Operation::Wait) {
        meaningfulMessage = (boost::format("SCA_SEQUENCE WAIT waitTime=%d cardSequence=%d link=%d error='%s'") % boost::get<WaitTime>(data) % mLink.cardSequence % mLink.linkId % e.what()).str();
      } else {
        meaningfulMessage = (boost::format("SCA_SEQUENCE UNKNOWN cardSequence=%d link=%d error='%s'") % mLink.cardSequence % mLink.linkId % e.what()).str();
      }
      Logger::get().err() << meaningfulMessage << endm;
      ret.push_back({ Operation::Error, meaningfulMessage });
      break;
    }
  }

  if (lock) {
    mLlaSession->stop();
  }

  return ret;
}

std::string Sca::writeSequence(const std::vector<std::pair<Operation, Data>>& operations, bool lock)
{
  std::stringstream resultBuffer;
  auto out = executeSequence(operations, lock);
  for (const auto& it : out) {
    Operation operation = it.first;
    Data data = it.second;
    if (operation == Operation::Command) {
      resultBuffer << data << "\n"; // "[cmd],[data]\n"
    } else if (operation == Operation::Wait) {
      resultBuffer << data << "\n"; // "[time]\n"
    } else if (operation == Operation::Error) {
      resultBuffer << data; // "[error_msg]"
      Logger::get().err() << data << endm;
      BOOST_THROW_EXCEPTION(ScaException() << ErrorInfo::Message(resultBuffer.str()));
      break;
    }
  }

  return resultBuffer.str();
}

std::ostream& operator<<(std::ostream& output, const Sca::CommandData& commandData)
{
  output << Util::formatValue(commandData.command) << "," << Util::formatValue(commandData.data);
  return output;
}

} // namespace alf
} // namespace o2
