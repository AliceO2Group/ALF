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

#include "AlfException.h"
#include "Logger.h"
#include "Ic/Ic.h"
#include "Util.h"

namespace roc = AliceO2::roc;
namespace sc_regs = AliceO2::roc::Cru::ScRegisters;

namespace AliceO2
{
namespace Alf
{

namespace ic_regs
{
static constexpr roc::Register IC_BASE(0x00f00000);
static constexpr roc::Register IC_WR_DATA(IC_BASE.address + 0x20);
static constexpr roc::Register IC_WR_CFG(IC_BASE.address + 0x24);
static constexpr roc::Register IC_WR_CMD(IC_BASE.address + 0x28);
static constexpr roc::Register IC_RD_DATA(IC_BASE.address + 0x30);
} // namespace ic_regs

Ic::Ic(AlfLink link) : mBar2(*link.bar2), mLink(link)
{
  setChannel(mLink.linkId);
  reset();

  // Set CFG to 0x3 by default
  barWrite(ic_regs::IC_WR_CFG.index, 0x3);
}

void Ic::setChannel(int gbtChannel)
{
  barWrite(sc_regs::SC_LINK.index, gbtChannel);
}

void Ic::reset()
{
  barWrite(sc_regs::SC_RESET.index, 0x1);
  barWrite(sc_regs::SC_RESET.index, 0x0); //void cmd to sync clocks
}

uint32_t Ic::read(uint32_t address)
{
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

void Ic::barWrite(uint32_t index, uint32_t data)
{
  mBar2.writeRegister(index, data);
}

uint32_t Ic::barRead(uint32_t index)
{
  uint32_t read = mBar2.readRegister(index);
  return read;
}

std::string Ic::writeSequence(std::vector<std::pair<IcData, Operation>> ops)
{
  std::stringstream resultBuffer;
  for (const auto& it : ops) {
    IcData icData = it.first;
    uint32_t ret;
    try {
      if (it.second == Operation::Read) {
        ret = read(icData);
        resultBuffer << Util::formatValue(ret) << "\n";
      } else if (it.second == Operation::Write) {
        ret = write(icData);
        resultBuffer << Util::formatValue(ret) << "\n";
      } else {
        BOOST_THROW_EXCEPTION(IcException() << ErrorInfo::Message("IC operation type unknown"));
      }
    } catch (const SwtException& e) {
      // If an IC error occurs, we stop executing the sequence of commands and return the results as far as we got them, plus
      // the error message.
      std::string meaningfulMessage = (boost::format("ic_regs::IC_SEQUENCE address=0x%08x data=0x%08x cruSequence=%d link=%d, error='%s'") % icData.address % icData.data % mLink.cruSequence % mLink.linkId % e.what()).str();
      getErrorLogger() << meaningfulMessage << endm;
      resultBuffer << meaningfulMessage;
      BOOST_THROW_EXCEPTION(IcException() << ErrorInfo::Message(resultBuffer.str()));
    }
  }
  return resultBuffer.str();
}

} // namespace Alf
} // namespace AliceO2
