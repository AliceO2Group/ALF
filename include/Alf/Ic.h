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

#ifndef O2_ALF_INC_IC_H
#define O2_ALF_INC_IC_H

#include <boost/variant.hpp>

#include "Common.h"
#include "ReadoutCard/BarInterface.h"
#include "ReadoutCard/Parameters.h"

namespace roc = AliceO2::roc;

namespace o2
{
namespace alf
{

/// Class for IC Transactions with the CRU
class Ic
{
 public:
  /// Struct holding the IC address and data pair; useful to AlfServer
  struct IcData {
    uint32_t address;
    uint32_t data = 0x0;
  };

  typedef uint32_t IcOut;
  typedef boost::variant<IcData, IcOut, std::string> Data;

  /// Internal constructor for the ALF server
  /// \param link AlfLink holding useful information coming from the AlfServer class
  Ic(AlfLink link);

  /// External constructor
  /// \param cardId The card ID for which to get the IC handle.
  /// \param linkId The link ID for which to get the IC handle.
  Ic(const roc::Parameters::CardIdType& cardId, int linkId);

  /// External constructor
  /// \param cardId The card ID for which to get the IC handle.
  /// \param linkId The link ID for which to get the IC handle.
  Ic(std::string cardId, int linkId);

  /// Performs an IC read
  /// \param address IC address to read from
  uint32_t read(uint32_t address);

  /// Performs an IC read
  /// \param icData Data struct containing the address to read from
  uint32_t read(IcData icData)
  {
    return read(icData.address);
  }

  /// Performs an IC write
  /// \param address IC address to write to
  /// \param data Data to write
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
                   Error };

  std::vector<std::pair<Operation, Data>> executeSequence(std::vector<std::pair<Operation, Data>> ops);

  /// Executes an IC sequence
  /// \param ops A vector of Data and Operations pairs
  /// \return A string of newline separated results for each operation
  /// \throws o2::alf::IcException on operation error
  std::string writeSequence(std::vector<std::pair<Operation, Data>> ops);

 private:
  void init(const roc::Parameters::CardIdType& cardId, int linkId);
  void reset();
  void setChannel(int gbtChannel);
  void barWrite(uint32_t offset, uint32_t data);
  uint32_t barRead(uint32_t index);

  std::shared_ptr<roc::BarInterface> mBar2;

  AlfLink mLink;
};

} // namespace alf
} // namespace o2

#endif // O2_ALF_INC_IC_H
