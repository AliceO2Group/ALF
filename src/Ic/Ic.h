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

/// \file Ic.h
/// \brief Definition of IC operations
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef ALICEO2_ALF_SRC_IC_IC_H
#define ALICEO2_ALF_SRC_IC_IC_H

#include "Common.h"
#include "ReadoutCard/RegisterReadWriteInterface.h"

namespace roc = AliceO2::roc;

namespace AliceO2
{
namespace Alf
{

/*namespace IcRegisters
{
  static constexpr Register IC_BASE(0x0f000000);
  static constexpr Register IC_WR_DATA(0x0f000020);
  static constexpr Register IC_WR_CFG(0x0f000024);
  static constexpr Register IC_WR_CMD(0x0f000028);
  static constexpr Register IC_RD_DATA(0x0f000030);
} // namespace IcRegisters*/

/// Class for IC(TODO:?) Transactions with the CRU
class Ic
{
 public:
  struct IcData {
    uint32_t address;
    uint32_t data;
  };

  Ic(AlfLink link);

  void setChannel(int gbtChannel);
  void reset();
  uint32_t read(uint32_t address);
  uint32_t read(IcData icData)
  {
    return read(icData.address);
  }
  uint32_t write(uint32_t address, uint32_t data);
  uint32_t write(IcData icData)
  {
    return write(icData.address, icData.data);
  }

  void writeGbtI2c(uint32_t data);
  enum Operation { Read,
                   Write };

  std::string writeSequence(std::vector<std::pair<IcData, Operation>> ops);

 private:
  void barWrite(uint32_t offset, uint32_t data);
  uint32_t barRead(uint32_t index);

  roc::RegisterReadWriteInterface& mBar2;

  AlfLink mLink;

  uint32_t IC_BASE;
  uint32_t IC_WR_DATA;
  uint32_t IC_WR_CFG;
  uint32_t IC_WR_CMD;
  uint32_t IC_RD_DATA;
};

} // namespace Alf
} // namespace AliceO2

#endif // ALICEO2_ALF_SRC_IC_IC_H
