
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

/// \file ScaMftPsu.cxx
/// \brief Implementation of ALICE Lowlevel Frontend (ALF) SCA MFT PSU operations
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
#include "Alf/ScaMftPsu.h"

#include "Logger.h"
#include "Util.h"

namespace sc_regs = AliceO2::roc::Cru::ScRegisters;

namespace o2
{
namespace alf
{

ScaMftPsu::ScaMftPsu(AlfLink link, std::shared_ptr<lla::Session> llaSession)
  : mLink(link), mBar2(mLink.bar)
{
  if (kDebugLogging) {
    Logger::setFacility("ALF/SCA_MFT_PSU");
  }
  mLlaSession = std::make_unique<LlaSession>(llaSession);
  mLink.rawLinkId = mLink.serialId.getEndpoint() * kCruNumLinks + mLink.linkId;
}

void ScaMftPsu::scReset()
{
  barWrite(sc_regs::SC_RESET.index, 0x1);
  barWrite(sc_regs::SC_RESET.index, 0x0); //void cmd to sync clocks
}

void ScaMftPsu::svlReset()
{
  barWrite((sc_regs::SCA_MFT_PSU_CTRL.address + 0x100 * mLink.rawLinkId) / 4, 0x1);
  barWrite((sc_regs::SCA_MFT_PSU_CTRL.address + 0x100 * mLink.rawLinkId) / 4, 0x0);
}

void ScaMftPsu::svlConnect()
{
  barWrite((sc_regs::SCA_MFT_PSU_CTRL.address + 0x100 * mLink.rawLinkId) / 4, 0x2);
  barWrite((sc_regs::SCA_MFT_PSU_CTRL.address + 0x100 * mLink.rawLinkId) / 4, 0x0);
}

void ScaMftPsu::setMaster()
{
  barWrite((sc_regs::SCA_MFT_PSU_MASTER_SLAVE.address + 0x100 * mLink.rawLinkId) / 4, 0x0);
}

void ScaMftPsu::setSlave()
{
  barWrite((sc_regs::SCA_MFT_PSU_MASTER_SLAVE.address + 0x100 * mLink.rawLinkId) / 4, 0x1);
}

ScaMftPsu::CommandData ScaMftPsu::executeCommand(uint32_t command, uint32_t data, bool lock)
{
  if (lock) {
    mLlaSession->start();
  }

  CommandData result;
  try {
    write(command, data);
    result = read();
  } catch (const ScaMftPsuException& e) {
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

void ScaMftPsu::write(uint32_t command, uint32_t data)
{
  waitOnBusyClear();
  barWrite((sc_regs::SCA_MFT_PSU_DATA.address + 0x100 * mLink.rawLinkId) / 4, data);
  barWrite((sc_regs::SCA_MFT_PSU_CMD.address + 0x100 * mLink.rawLinkId) / 4, command);
  auto transactionId = (command >> 16) & 0xff;
  if (transactionId == 0x0 || transactionId == 0xff) {
    BOOST_THROW_EXCEPTION(ScaMftPsuException()
                          << ErrorInfo::Message("Invalid transaction ID"));
  }

  try {
    execute();
  } catch (const ScaMftPsuException& e) {
    throw;
  }
}

ScaMftPsu::CommandData ScaMftPsu::read()
{
  waitOnBusyClear();
  auto data = barRead((sc_regs::SCA_MFT_PSU_DATA.address + 0x100 * mLink.rawLinkId) / 4);
  auto command = barRead((sc_regs::SCA_MFT_PSU_CMD.address + 0x100 * mLink.rawLinkId) / 4);
  // printf("ScaMftPsu::read   DATA=0x%x   CH=0x%x   TR=0x%x   CMD=0x%x\n", data,
  // command >> 24, (command >> 16) & 0xff, command & 0xff);

  auto endTime = std::chrono::steady_clock::now() + CHANNEL_BUSY_TIMEOUT;
  while (std::chrono::steady_clock::now() < endTime) {
    if (!isChannelBusy(command)) {
      checkError(command);
      return { command, data };
    }
    data = barRead((sc_regs::SCA_MFT_PSU_DATA.address + 0x100 * mLink.rawLinkId) / 4);
    command = barRead((sc_regs::SCA_MFT_PSU_CMD.address + 0x100 * mLink.rawLinkId) / 4);
  }

  std::stringstream ss;
  ss << "command: " << command << " data: " << data;
  BOOST_THROW_EXCEPTION(ScaMftPsuException() << ErrorInfo::Message(
                          "Exceeded timeout on channel busy wait" + ss.str()));
}

bool ScaMftPsu::isChannelBusy(uint32_t command)
{
  return (command & 0xff) == 0x40;
}

void ScaMftPsu::checkError(uint32_t command)
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

    BOOST_THROW_EXCEPTION(ScaMftPsuException() << ErrorInfo::Message(
                            stream.str()));
  }
}

void ScaMftPsu::barWrite(uint32_t index, uint32_t data)
{
  mBar2->writeRegister(index, data);
}

uint32_t ScaMftPsu::barRead(uint32_t index)
{
  return mBar2->readRegister(index);
}

void ScaMftPsu::execute()
{
  barWrite((sc_regs::SCA_MFT_PSU_CTRL.address + 0x100 * mLink.rawLinkId) / 4, 0x4);
  barWrite((sc_regs::SCA_MFT_PSU_CTRL.address + 0x100 * mLink.rawLinkId) / 4, 0x0);
  waitOnBusyClear();
}

void ScaMftPsu::waitOnBusyClear()
{
  auto endTime = std::chrono::steady_clock::now() + BUSY_TIMEOUT;
  while (std::chrono::steady_clock::now() < endTime) {
    if ((((barRead((sc_regs::SCA_MFT_PSU_CTRL.address + 0x100 * mLink.rawLinkId) / 4)) >> 31) & 0x1) == 0) {
      return;
    }
  }

  BOOST_THROW_EXCEPTION(ScaMftPsuException()
                        << ErrorInfo::Message("Exceeded timeout on busy wait"));
}

std::vector<std::pair<ScaMftPsu::Operation, ScaMftPsu::Data>> ScaMftPsu::executeSequence(const std::vector<std::pair<Operation, Data>>& operations, bool lock)
{
  if (lock) {
    mLlaSession->start();
  }

  std::vector<std::pair<ScaMftPsu::Operation, ScaMftPsu::Data>> ret;
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
      } else if (operation == Operation::SVLReset) {
        svlReset();
        ret.push_back({ Operation::SVLReset, {} });
      } else if (operation == Operation::SCReset) {
        scReset();
        ret.push_back({ Operation::SCReset, {} });
      } else if (operation == Operation::SVLConnect) {
        svlConnect();
        ret.push_back({ Operation::SVLConnect, {} });
      } else if (operation == Operation::Master) {
        setMaster();
        ret.push_back({ Operation::Master, {} });
      } else if (operation == Operation::Slave) {
        setSlave();
        ret.push_back({ Operation::Slave, {} });
      } else {
        BOOST_THROW_EXCEPTION(ScaMftPsuException() << ErrorInfo::Message("SCA operation type unknown"));
      }
    } catch (const ScaMftPsuException& e) {
      // If an SCA error occurs, we stop executing the sequence of commands and return the results as far as we got
      // them, plus the error message.
      std::string meaningfulMessage;
      if (operation == Operation::Command) {
        meaningfulMessage = (boost::format("SCA_SEQUENCE cmd=0x%08x data=0x%08x serialId=%s link=%d error='%s'") % boost::get<CommandData>(data).command % boost::get<CommandData>(data).data % mLink.serialId % mLink.linkId % e.what()).str();
      } else if (operation == Operation::Wait) {
        meaningfulMessage = (boost::format("SCA_SEQUENCE WAIT waitTime=%d serialId=%s link=%d error='%s'") % boost::get<WaitTime>(data) % mLink.serialId % mLink.linkId % e.what()).str();
      } else if (operation == Operation::SVLReset) {
        meaningfulMessage = (boost::format("SCA_SEQUENCE SVL RESET serialId=%s link=%d error='%s'") % mLink.serialId % mLink.linkId % e.what()).str();
      } else if (operation == Operation::SCReset) {
        meaningfulMessage = (boost::format("SCA_SEQUENCE SC RESET serialId=%s link=%d error='%s'") % mLink.serialId % mLink.linkId % e.what()).str();
      } else if (operation == Operation::SVLConnect) {
        meaningfulMessage = (boost::format("SCA_SEQUENCE SVL CONNECT serialId=%s link=%d error='%s'") % mLink.serialId % mLink.linkId % e.what()).str();
      } else if (operation == Operation::Master) {
        meaningfulMessage = (boost::format("SCA_SEQUENCE MASTER serialId=%s link=%d error='%s'") % mLink.serialId % mLink.linkId % e.what()).str();
      } else if (operation == Operation::Slave) {
        meaningfulMessage = (boost::format("SCA_SEQUENCE SLAVE serialId=%s link=%d error='%s'") % mLink.serialId % mLink.linkId % e.what()).str();
      } else {
        meaningfulMessage = (boost::format("SCA_SEQUENCE UNKNOWN serialId=%s link=%d error='%s'") % mLink.serialId % mLink.linkId % e.what()).str();
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

std::string ScaMftPsu::writeSequence(const std::vector<std::pair<Operation, Data>>& operations, bool lock)
{
  std::stringstream resultBuffer;
  auto out = executeSequence(operations, lock);
  for (const auto& it : out) {
    Operation operation = it.first;
    Data data = it.second;
    if (operation == Operation::Command) {
      resultBuffer << data << "\n"; // "[cmd],[data]\n"
    } else if (operation == Operation::Wait) {
      resultBuffer << std::dec << data << "\n"; // "[time]\n"
    } else if (operation == Operation::SVLReset || operation == Operation::SCReset) {
      // DO NOTHING
    } else if (operation == Operation::SVLConnect) {
      resultBuffer << "svl_connect\n"; // echo
    } else if (operation == Operation::Master) {
      resultBuffer << "master\n"; // echo
    } else if (operation == Operation::Slave) {
      resultBuffer << "slave\n"; // echo
    } else if (operation == Operation::Error) {
      resultBuffer << data; // "[error_msg]"
      if (kDebugLogging) {
        Logger::get() << data << LogErrorDevel << endm;
      }
      BOOST_THROW_EXCEPTION(ScaMftPsuException() << ErrorInfo::Message(resultBuffer.str()));
      break;
    }
  }

  return resultBuffer.str();
}

// static
bool ScaMftPsu::isAnMftPsuLink(AlfLink link) {
  return link.bar->readRegister(sc_regs::SCA_MFT_PSU_ID.index) == 0x1;
}

} // namespace alf
} // namespace o2
