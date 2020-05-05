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

#include "AlfException.h"
#include "Logger.h"
#include "ReadoutCard/Cru.h"
#include "Sca/Sca.h"
#include "Util.h"

namespace sc_regs = AliceO2::roc::Cru::ScRegisters;

namespace AliceO2
{
namespace Alf
{

// std::map<std::string, uint32_t> Sca::registers;

Sca::Sca(AlfLink link)
  : mBar2(*link.bar2), mLink(link)
{
  if (mLink.linkId >= CRU_NUM_LINKS) {
    BOOST_THROW_EXCEPTION(
      ScaException() << ErrorInfo::Message("Maximum link number exceeded"));
  }

  barWrite(sc_regs::SC_RESET.index, 0x1);
  barWrite(sc_regs::SC_RESET.index, 0x0);
  barWrite(sc_regs::SC_LINK.index, mLink.linkId);
}

// UNUSED
void Sca::initialize()
{
  init(); //TODO: handle error??
  gpioEnable();
}

// UNUSED
void Sca::init()
{
  barWrite(sc_regs::SCA_WR_CTRL.index, 0x1);
  waitOnBusyClear();
  barWrite(sc_regs::SCA_WR_CTRL.index, 0x2);
  waitOnBusyClear();
  barWrite(sc_regs::SCA_WR_CTRL.index, 0x0);
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
    executeCommand();
  } catch (const ScaException& e) {
    throw;
  }
}

Sca::ReadResult Sca::read()
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

// UNUSED
void Sca::gpioEnable()
{
  // Enable GPIO
  // WR CONTROL REG B
  write(0x00010002, 0xff000000);
  read();
  // RD CONTROL REG B
  write(0x00020003, 0xff000000);
  read();

  // WR GPIO DIR
  write(0x02030020, 0xffffffff);
  // RD GPIO DIR
  write(0x02040021, 0x0);
  read();
}

// UNUSED
Sca::ReadResult Sca::gpioWrite(uint32_t data)
{
  //  printf("Sca::gpioWrite DATA=0x%x\n", data);
  initialize();
  // WR REGISTER OUT DATA
  write(0x02040010, data);
  // RD DATA
  write(0x02050011, 0x0);
  read();
  // RD REGISTER DATAIN
  write(0x02060001, 0x0);
  return read();
}

// UNUSED
Sca::ReadResult Sca::gpioRead()
{
  // printf("Sca::gpioRead\n", data);
  // RD DATA
  write(0x02050011, 0x0);
  return read();
}

void Sca::barWrite(uint32_t index, uint32_t data)
{
  mBar2.writeRegister(index, data);
}

uint32_t Sca::barRead(uint32_t index)
{
  return mBar2.readRegister(index);
}

void Sca::executeCommand()
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

std::string Sca::writeSequence(const std::vector<std::pair<Data, Operation>>& operations)
{
  std::stringstream resultBuffer;
  for (const auto& it : operations) {
    Data data = it.first;
    try {
      if (it.second == Operation::Command) {
        auto commandData = std::get<CommandData>(data);
        write(commandData);
        ReadResult result = read();
        resultBuffer << Util::formatValue(commandData.command) << "," << Util::formatValue(result.data) << "\n";
      } else if (it.second == Operation::Wait) {
        int waitTime;
        try {
          waitTime = std::get<WaitTime>(data);
        } catch (...) { // no timeout was provided
          data = DEFAULT_SCA_WAIT_TIME_MS;
          waitTime = std::get<WaitTime>(data);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(waitTime));
        resultBuffer << waitTime << "\n";
      } else {
        BOOST_THROW_EXCEPTION(ScaException() << ErrorInfo::Message("SCA operation type unknown"));
      }
    } catch (const ScaException& e) {
      // If an SCA error occurs, we stop executing the sequence of commands and return the results as far as we got
      // them, plus the error message.
      std::string meaningfulMessage;
      if (it.second == Operation::Command) {
        meaningfulMessage = (boost::format("SCA_SEQUENCE cmd=0x%08x data=0x%08x cruSequence=%d link=%d error='%s'") % std::get<Command>(data).command % std::get<Command>(data).data % mLink.cruSequence % mLink.linkId % e.what()).str();
      } else if (it.second == Operation::Wait) {
        meaningfulMessage = (boost::format("SCA_SEQUENCE WAIT waitTime=%d cruSequence=%d link=%d error='%s'") % std::get<WaitTime>(data) % mLink.cruSequence % mLink.linkId % e.what()).str();
      } else {
        meaningfulMessage = (boost::format("SCA_SEQUENCE UNKNOWN cruSequence=%d link=%d error='%s'") % mLink.cruSequence % mLink.linkId % e.what()).str();
      }
      getErrorLogger() << meaningfulMessage << endm;
      resultBuffer << meaningfulMessage;
      BOOST_THROW_EXCEPTION(ScaException() << ErrorInfo::Message(resultBuffer.str()));
    }
  }

  return resultBuffer.str();
}

} // namespace Alf
} // namespace AliceO2
