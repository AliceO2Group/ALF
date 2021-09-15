
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

/// \file Sca.h
/// \brief Definition of ALICE Lowlevel Frontend (ALF) SCA operations
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef O2_ALF_INC_SCA_H
#define O2_ALF_INC_SCA_H

#include <string>
#include <map>
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

/// Class for interfacing with the C-RORC's(?) and CRU's Slow-Control Adapter (SCA)
class Sca : public ScBase
{
 public:
  /// Struct holding the command and data pair of an SCA command
  struct CommandData {
    uint32_t command;
    uint32_t data;
  };

  typedef int WaitTime;
  /// Typedef for the Data type of an SCA sequence operation.
  /// Variant of CommandData for writes, WaitTime for waits, std::string for errors;
  typedef boost::variant<CommandData, WaitTime, std::string> Data;

  /// Enum for the different SCA operation types as seen from DIM RPCs
  enum Operation { Command,
                   Wait,
                   SCReset,
                   SVLReset,
                   SVLConnect,
                   Error,
                   Lock,
                   Master,
                   Slave };

  /// Internal constructor for the AlfServer
  /// \param link AlfLink holding useful information coming from the AlfServer class
  Sca(AlfLink link, std::shared_ptr<lla::Session> llaSession);

  /// External constructor
  /// \param cardId The card ID for which to get the SCA handle.
  /// \param linkId The link ID to set the channel to (optional).
  Sca(const roc::Parameters::CardIdType& cardId, int linkId = -1);

  /// External constructor
  /// \param cardId The card ID for which to get the SCA handle.
  /// \param linkId The link ID to set the channel to (optional).
  Sca(std::string cardId, int linkId = -1);

  /// Executes an SCA reset
  void svlReset();

  /// Executes an SCA connect
  void svlConnect();

  /// Executes an SCA command
  /// \param commandData SCA command, data pair
  /// \param lock Boolean enabling implicit locking
  /// \throws o2::lla::LlaException on lock fail,
  ///         o2::alf::ScaException on SCA error
  CommandData executeCommand(CommandData commandData, bool lock = false)
  {
    return executeCommand(commandData.command, commandData.data, lock);
  }
  /// Executes an SCA command
  /// \param command SCA command
  /// \param data SCA data
  /// \param lock Boolean enabling implicit locking
  /// \throws  o2::lla::LlaException on lock fail
  ///          o2::alf::ScaException on SCA error
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
  ///         o2::alf::ScaException on invalid operation or error
  std::string writeSequence(const std::vector<std::pair<Operation, Data>>& operations, bool lock = false);

  static std::string ScaOperationToString(Operation op);
  static Sca::Operation StringToScaOperation(std::string op);

 private:
  /// Performs an SCA read
  /// \return CommandData An SCA command, data pair
  /// \throws o2::alf::ScaException on SCA error
  CommandData read();

  /// Performs an SCA write
  /// \param command SCA command
  /// \param data SCA data
  /// \throws o2::alf::ScaException on SCA error
  void write(uint32_t command, uint32_t data);

  /// Performs an SCA write
  /// \param commandData SCA command, data pair
  /// \throws o2::alf::ScaException on SCA error
  void write(CommandData commandData)
  {
    write(commandData.command, commandData.data);
  };
  void execute();
  void checkError(uint32_t command);
  bool isChannelBusy(uint32_t command);
  void waitOnBusyClear();

  static constexpr int DEFAULT_SCA_WAIT_TIME_MS = 3;
};

std::ostream& operator<<(std::ostream& output, const Sca::CommandData& commandData);

} // namespace alf
} // namespace o2

#endif // O2_ALF_INC_SCA_H
