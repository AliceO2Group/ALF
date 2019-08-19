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

namespace AliceO2
{
namespace Alf
{

Ic::Ic(AlfLink link) : mBar2(*link.bar2), mLink(link)
{
  switch (mLink.linkId) {
    //case 0 : IC_BASE = 0x0422400; // Came from cru-sw like this
    case 1: IC_BASE = 0x0424400;
             break;
    case 2: IC_BASE = 0x0426400;
             break;
    case 3: IC_BASE = 0x0428400;
             break;
    default: IC_BASE = 0x0422400;
              break;
  }

  IC_WR_DATA = IC_BASE + 0x20;
  IC_WR_CFG = IC_BASE + 0x24;
  IC_WR_CMD = IC_BASE + 0x28;
  IC_RD_DATA = IC_BASE + 0x30;

  // Set CFG to 0x7 by default
  barWrite(IC_WR_CFG / 4, 0x7);
}

void Ic::reset()
{
}

uint32_t Ic::read(uint32_t address)
{
  address = address & 0xffff;
  //uint32_t data = (data & 0xff) << 16; //TODO: DATA HERE SHOULD BE 0!
  uint32_t data = 0; //TODO: DATA HERE SHOULD BE 0!

  data = data + address;

  // Write to the FIFO
  barWrite(IC_WR_DATA / 4, data);
  barWrite(IC_WR_CMD / 4, 0x1);
  barWrite(IC_WR_CMD / 4, 0x0);
  
  // Execute the RD SM (TODO: ?)
  barWrite(IC_WR_CMD / 4, 0x8);
  barWrite(IC_WR_CMD / 4, 0x0);

  // Pulse the READ
  barWrite(IC_WR_CMD / 4, 0x2);
  barWrite(IC_WR_CMD / 4, 0x0);

  // Read the status of the FIFO
  uint32_t ret = barRead(IC_RD_DATA / 4);
  //uint32_t gbtAddress = (ret >> 8) & 0xff;
  uint32_t retData = ret & 0xff;
  //uint32_t empty = (ret >> 16) & 0x1;
  //uint32_t ready = (ret >> 31) & 0x1;

  return retData;
}

uint32_t Ic::write(uint32_t address, uint32_t data)
{
  address = address & 0xffff;
  data = (data & 0xf) << 16;

  data += address;

  // Write to the FIFO
  barWrite(IC_WR_DATA / 4, data);
  barWrite(IC_WR_CMD / 4, 0x1);
  barWrite(IC_WR_CMD / 4, 0x0);

  // Execute the WR SM (TODO: ?)
  barWrite(IC_WR_CMD / 4, 0x4);
  barWrite(IC_WR_CMD / 4, 0x0);

  // Read the status of the FIFO
  uint32_t ret = barRead(IC_RD_DATA / 4);
  //uint32_t gbtAddress = (ret >> 8) & 0xff;
  uint32_t retData = ret & 0xff;
  uint32_t empty = (ret >> 16) & 0x1;
  uint32_t ready = (ret >> 31) & 0x1;
  
  if (empty != 0x0 || ready != 0x1) {
    BOOST_THROW_EXCEPTION(IcException() << ErrorInfo::Message("IC WRITE was unsuccesful"));
  }
  return retData;
}

void Ic::writeGbtI2c(uint32_t data)
{
  barWrite(IC_WR_CFG / 4, data);
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
        resultBuffer << ret << "\n";
      } else if (it.second == Operation::Write) {
        ret = write(icData);
        resultBuffer << ret << "\n";
      } else {
        BOOST_THROW_EXCEPTION(IcException() << ErrorInfo::Message("IC operation type unknown"));
      }
    } catch (const SwtException& e) {
      // If an IC error occurs, we stop executing the sequence of commands and return the results as far as we got them, plus
      // the error message.
      std::string meaningfulMessage = (boost::format("IC_SEQUENCE address=0x%08x data=0x%08x serial=%d link=%d, error='%s'") % icData.address % icData.data % mLink.serial % mLink.linkId % e.what()).str();
      getErrorLogger() << meaningfulMessage << endm;
      resultBuffer << meaningfulMessage;
      BOOST_THROW_EXCEPTION(IcException() << ErrorInfo::Message(resultBuffer.str()));
    }
  }
  return resultBuffer.str();

}

} // namespace Alf
} // namespace AliceO2
