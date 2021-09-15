
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

/// \file ScaMftPsu.h
/// \brief Definition of ALICE Lowlevel Frontend (ALF) SCA operations for the MFT PSU
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef O2_ALF_INC_SCA_MFT_PSU_H
#define O2_ALF_INC_SCA_MFT_PSU_H

#include <string>
#include <map>
#include <boost/variant.hpp>

#include "ReadoutCard/BarInterface.h"
#include "ReadoutCard/Parameters.h"

#include "Common.h"
#include "Alf/Lla.h"
#include "Alf/Sca.h"
#include "Alf/ScaMftPsu.h"

namespace roc = AliceO2::roc;

namespace o2
{
namespace alf
{

/// Class for interfacing with the MFT PSU Slow-Control Adapter (SCA)
class ScaMftPsu
{
 public:
  // Pull these from Sca to simplify things a bit
  typedef Sca::CommandData CommandData;
  typedef Sca::WaitTime WaitTime;
  typedef Sca::Data Data;
  typedef Sca::Operation Operation;

  /// Internal constructor for the AlfServer
  /// \param link AlfLink holding useful information coming from the AlfServer class
  ScaMftPsu(AlfLink link, std::shared_ptr<lla::Session> llaSession);

  /// Executes a global SC reset
  void scReset();

  /// Executes an SCA reset
  void svlReset();

  /// Executes an SCA connect
  void svlConnect();

  /// Changes to master
  void setMaster();

  /// Changes to slave
  void setSlave();

  /// Executes an SCA command
  /// \param commandData SCA command, data pair
  /// \param lock Boolean enabling implicit locking
  /// \throws o2::lla::LlaException on lock fail,
  ///         o2::alf::ScaMftPsuException on SCA error
  CommandData executeCommand(CommandData commandData, bool lock = false)
  {
    return executeCommand(commandData.command, commandData.data, lock);
  }
  /// Executes an SCA command
  /// \param command SCA command
  /// \param data SCA data
  /// \param lock Boolean enabling implicit locking
  /// \throws  o2::lla::LlaException on lock fail
  ///          o2::alf::ScaMftPsuException on SCA error
  CommandData executeCommand(uint32_t command, uint32_t data, bool lock = false);

  /// Executes an SCA sequence
  /// \param operations A vector of Operation and Data pairs
  /// \param lock Boolean enabling implicit locking
  /// \return A vector of Operation and Data pairs
  ///         CommandData for Commands
  ///         WaitTime for Waits
  ///         std::string for Errors
  /// \throws o2::lla::LlaException on lock fail
  std::vector<std::pair<Operation, Data>> executeSequence(const std::vector<std::pair<Operation, Data>>& operations, bool lock = false);

  /// Executes an SCA sequence for the ALF Server
  /// \param operations A vector of Data and Operation pairs
  /// \param lock Boolean enabling implicit locking
  /// \return A string of newline separated results;
  /// \throws o2::lla::LlaException on lock fail
  ///         o2::alf::ScaMftPsuException on invalid operation or error
  std::string writeSequence(const std::vector<std::pair<Operation, Data>>& operations, bool lock = false);

  /// Checks if the link should be used for the MFT PSU service
  /// \param link The AlfLink to check
  /// \return A bool if the link should be used for the MFT PSU service
  static bool isAnMftPsuLink(AlfLink link);

 private:
  uint32_t barRead(uint32_t index);
  void barWrite(uint32_t index, uint32_t data);

  /// Performs an SCA read
  /// \return CommandData An SCA command, data pair
  /// \throws o2::alf::ScaMftPsuException on SCA error
  CommandData read();

  /// Performs an SCA write
  /// \param command SCA command
  /// \param data SCA data
  /// \throws o2::alf::ScaMftPsuException on SCA error
  void write(uint32_t command, uint32_t data);

  /// Performs an SCA write
  /// \param commandData SCA command, data pair
  /// \throws o2::alf::ScaMftPsuException on SCA error
  void write(CommandData commandData)
  {
    write(commandData.command, commandData.data);
  };
  void execute();
  void checkError(uint32_t command);
  bool isChannelBusy(uint32_t command);
  void waitOnBusyClear();

  /// Interface for BAR 2
  AlfLink mLink;
  std::shared_ptr<roc::BarInterface> mBar2;
  std::unique_ptr<LlaSession> mLlaSession;

  static constexpr int DEFAULT_SCA_WAIT_TIME_MS = 3;
};

} // namespace alf
} // namespace o2

#endif // O2_ALF_INC_SCA_H
