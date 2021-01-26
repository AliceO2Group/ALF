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

/// \file Ic.cxx
/// \brief Definition of IC operations
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <boost/format.hpp>
#include <chrono>
#include <thread>

#include "ReadoutCard/CardFinder.h"
#include "ReadoutCard/ChannelFactory.h"

#include "Alf/Exception.h"
#include "Logger.h"
#include "Alf/Ic.h"
#include "Util.h"

namespace roc = AliceO2::roc;
namespace sc_regs = AliceO2::roc::Cru::ScRegisters;

namespace o2
{
namespace alf
{

namespace ic_regs
{
static constexpr roc::Register IC_BASE(0x00f00000);
static constexpr roc::Register IC_WR_DATA(IC_BASE.address + 0x20);
static constexpr roc::Register IC_WR_CFG(IC_BASE.address + 0x24);
static constexpr roc::Register IC_WR_CMD(IC_BASE.address + 0x28);
static constexpr roc::Register IC_RD_DATA(IC_BASE.address + 0x30);
} // namespace ic_regs

Ic::Ic(AlfLink link, std::shared_ptr<lla::Session> llaSession)
  : ScBase(link, llaSession)
{
  Logger::setFacility("ALF/IC");

  // Set CFG to 0x3 by default
  barWrite(ic_regs::IC_WR_CFG.index, 0x3);
}

Ic::Ic(const roc::Parameters::CardIdType& cardId, int linkId)
  : ScBase(cardId, linkId)
{
  Logger::setFacility("ALF/IC");

  // Set CFG to 0x3 by default
  barWrite(ic_regs::IC_WR_CFG.index, 0x3);
}

Ic::Ic(std::string cardId, int linkId)
  : ScBase(cardId, linkId)
{
  Logger::setFacility("ALF/IC");

  // Set CFG to 0x3 by default
  barWrite(ic_regs::IC_WR_CFG.index, 0x3);
}

uint32_t Ic::read(uint32_t address)
{
  checkChannelSet();

  address = address & 0xffff;
  uint32_t data = 0;

  data = data + address;

  // Write to the FIFO
  barWrite(ic_regs::IC_WR_DATA.index, data);
  barWrite(ic_regs::IC_WR_CMD.index, 0x1);
  barWrite(ic_regs::IC_WR_CMD.index, 0x0);

  // Execute the RD State Machine
  barWrite(ic_regs::IC_WR_CMD.index, 0x8);
  barWrite(ic_regs::IC_WR_CMD.index, 0x0);

  // Pulse the READ
  barWrite(ic_regs::IC_WR_CMD.index, 0x2);
  barWrite(ic_regs::IC_WR_CMD.index, 0x0);

  // Read the status of the FIFO
  uint32_t ret = barRead(ic_regs::IC_RD_DATA.index);
  //uint32_t gbtAddress = (ret >> 8) & 0xff;
  uint32_t retData = ret & 0xff;
  //uint32_t empty = (ret >> 16) & 0x1;
  //uint32_t ready = (ret >> 31) & 0x1;

  return retData;
}

uint32_t Ic::write(uint32_t address, uint32_t data)
{
  checkChannelSet();

  uint32_t echo = data;
  address = address & 0xffff;
  data = (data & 0xff) << 16;

  data += address;

  // Write to the FIFO
  barWrite(ic_regs::IC_WR_DATA.index, data);
  barWrite(ic_regs::IC_WR_CMD.index, 0x1);
  barWrite(ic_regs::IC_WR_CMD.index, 0x0);

  // Execute the WR State Machine
  barWrite(ic_regs::IC_WR_CMD.index, 0x4);
  barWrite(ic_regs::IC_WR_CMD.index, 0x0);

  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // Read the status of the FIFO
  uint32_t ret = barRead(ic_regs::IC_RD_DATA.index);
  //uint32_t gbtAddress = (ret >> 8) & 0xff;
  //uint32_t retData = ret & 0xff;
  uint32_t empty = (ret >> 16) & 0x1;
  uint32_t ready = (ret >> 31) & 0x1;

  if (empty != 0x0 || ready != 0x1) {
    BOOST_THROW_EXCEPTION(IcException() << ErrorInfo::Message("IC WRITE was unsuccesful"));
  }
  return echo;
}

void Ic::writeGbtI2c(uint32_t data)
{
  barWrite(ic_regs::IC_WR_CFG.index, data);
}

std::vector<std::pair<Ic::Operation, Ic::Data>> Ic::executeSequence(std::vector<std::pair<Operation, Data>> ops, bool lock)
{
  if (lock) {
    mLlaSession->start();
  }

  // force set the channel within the atomic part of the sequence
  // to be changed as soon as FW provides set channel
  try {
    checkChannelSet();
  } catch (const IcException& e) {
    return { { Operation::Error, e.what() } };
  }

  std::vector<std::pair<Ic::Operation, Ic::Data>> ret;
  for (const auto& it : ops) {
    Operation operation = it.first;
    Data data = it.second;
    try {
      if (operation == Operation::Read) {
        auto out = read(boost::get<uint32_t>(data));
        ret.push_back({ operation, out });
      } else if (operation == Operation::Write) {
        write(boost::get<IcData>(data));
        ret.push_back({ operation, IcData{ boost::get<IcData>(data) } });
      } else {
        BOOST_THROW_EXCEPTION(IcException() << ErrorInfo::Message("IC operation type unknown"));
      }
    } catch (const IcException& e) {
      // If an IC error occurs, we stop executing the sequence of commands and return the results as far as we got them, plus
      // the error message.
      IcData icData = boost::get<IcData>(data);
      std::string meaningfulMessage = (boost::format("ic_regs::IC_SEQUENCE address=0x%08x data=0x%08x serialId=%s link=%d, error='%s'") % icData.address % icData.data % mLink.serialId % mLink.linkId % e.what()).str();
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

std::string Ic::writeSequence(std::vector<std::pair<Operation, Data>> ops, bool lock)
{
  std::stringstream resultBuffer;
  auto out = executeSequence(ops, lock);
  for (const auto& it : out) {
    Operation operation = it.first;
    Data data = it.second;
    if (operation == Operation::Read || operation == Operation::Write) {
      resultBuffer << Util::formatValue(boost::get<IcOut>(data)) << "\n";
    } else if (operation == Operation::Error) {
      std::string errMessage = boost::get<std::string>(data);
      resultBuffer << errMessage;
      Logger::get() << errMessage << LogErrorDevel << endm;
      BOOST_THROW_EXCEPTION(IcException() << ErrorInfo::Message(resultBuffer.str()));
    }
  }

  return resultBuffer.str();
}

std::string Ic::IcOperationToString(Ic::Operation op)
{
  if (op == Ic::Operation::Read) {
    return "read";
  } else if (op == Ic::Operation::Write) {
    return "write";
  } else if (op == Ic::Operation::Error) {
    return "error";
  } else if (op == Ic::Operation::Lock) {
    return "lock";
  }

  BOOST_THROW_EXCEPTION(IcException() << ErrorInfo::Message("Cannot convert Ic operation to string"));
}

Ic::Operation Ic::StringToIcOperation(std::string op)
{
  if (op == "read") {
    return Ic::Operation::Read;
  } else if (op == "write") {
    return Ic::Operation::Write;
  } else if (op == "error") {
    return Ic::Operation::Error;
  } else if (op == "lock") {
    return Ic::Operation::Lock;
  }

  BOOST_THROW_EXCEPTION(IcException() << ErrorInfo::Message("Cannot convert IC operation to string " + op));
}

} // namespace alf
} // namespace o2
