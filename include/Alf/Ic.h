
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

/// \file Ic.h
/// \brief Definition of IC operations
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef O2_ALF_INC_IC_H
#define O2_ALF_INC_IC_H

#include <boost/variant.hpp>

#include "ReadoutCard/BarInterface.h"
#include "ReadoutCard/Parameters.h"

#include "Common.h"
#include "Alf/Lla.h"
#include "Alf/ScBase.h"

namespace roc = AliceO2::roc;

namespace o2
{
namespace alf
{

/// Class for IC Transactions with the CRU
class Ic : public ScBase
{
 public:
  /// Struct holding the IC address and data pair; useful to AlfServer
  struct IcData {
    uint32_t address = 0x0;
    uint32_t data = 0x0;
  };

  typedef uint32_t IcOut;
  /// Typedef for the Data type of an IC sequence operation.
  /// Variant of IcData for writes, IcOut for reads, std::string for errors;
  typedef boost::variant<IcData, IcOut, std::string> Data;

  /// Internal constructor for the ALF server
  /// \param link AlfLink holding useful information coming from the AlfServer class
  Ic(AlfLink link, std::shared_ptr<lla::Session> llaSession);

  /// External constructor
  /// \param cardId The card ID for which to get the IC handle.
  /// \param linkId The link ID for which to get the IC handle.
  Ic(const roc::Parameters::CardIdType& cardId, int linkId = -1);

  /// External constructor
  /// \param cardId The card ID for which to get the IC handle.
  /// \param linkId The link ID for which to get the IC handle.
  Ic(std::string cardId, int linkId = -1);

  /// Performs an IC read
  /// \param address IC address to read from
  /// \return IC data requested
  uint32_t read(uint32_t address);

  /// Performs an IC read
  /// \param icData Data struct containing the address to read from
  /// \return echo of the IC data written
  uint32_t read(IcData icData)
  {
    return read(icData.address);
  }

  /// Performs an IC write
  /// \param address IC address to write to
  /// \param data Data to write
  /// \throws o2::alf::IcException on write failure
  uint32_t write(uint32_t address, uint32_t data);

  /// Performs an IC write
  /// \param icData IcData struct containg the data and the address to write to
  /// \throws o2::alf::IcException on write error
  uint32_t write(IcData icData)
  {
    return write(icData.address, icData.data);
  }

  /// Performs a GBT I2C write
  /// \param data Data to write
  void writeGbtI2c(uint32_t data);

  /// Enum for the different IC operation types
  enum Operation { Read,
                   Write,
                   Error,
                   Lock };

  /// Executes an IC sequence
  /// \param ops A vector of Data and Operations pairs
  /// \return A vector of Data and Operation pairs
  //          IcOut for Writes and Reads
  //          std::string for Errors
  /// \throws o2::lla::LlaException on lock fail
  std::vector<std::pair<Operation, Data>> executeSequence(std::vector<std::pair<Operation, Data>> ops, bool lock = false);

  /// Executes an IC sequence for the ALF server
  /// \param ops A vector of Data and Operations pairs
  /// \return A string of newline separated results for each operation
  /// \throws o2::alf::IcException on operation error
  ///         o2::lla::LlaException on lock fail
  std::string writeSequence(std::vector<std::pair<Operation, Data>> ops, bool lock = false);

  static std::string IcOperationToString(Operation op);
  static Ic::Operation StringToIcOperation(std::string op);
};

} // namespace alf
} // namespace o2

#endif // O2_ALF_INC_IC_H
