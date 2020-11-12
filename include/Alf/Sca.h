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

namespace roc = AliceO2::roc;

namespace o2
{
namespace alf
{

/// Class for interfacing with the C-RORC's(?) and CRU's Slow-Control Adapter (SCA)
class Sca
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
                   Lock };

  /// Internal constructor for the AlfServer
  /// \param link AlfLink holding useful information coming from the AlfServer class
  Sca(AlfLink link);

  /// External constructor
  /// \param cardId The card ID for which to get the SCA handle.
  /// \param linkId The link ID to set the channel to (optional).
  Sca(const roc::Parameters::CardIdType& cardId, int linkId = -1);

  /// External constructor
  /// \param cardId The card ID for which to get the SCA handle.
  /// \param linkId The link ID to set the channel to (optional).
  Sca(std::string cardId, int linkId = -1);

  /// Sets the SCA channel
  /// \param gbtChannel The channel to set
  void setChannel(int gbtChannel);

  /// Checks if an SCA channel has been selected
  /// \throws o2::alf::ScaException if no SCA channel selected
  void checkChannelSet();

  /// Executes a global SC reset
  void scReset();

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

 private:
  void init(const roc::Parameters::CardIdType& cardId, int linkId);

  uint32_t barRead(uint32_t index);
  void barWrite(uint32_t index, uint32_t data);

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

  /// Interface for BAR 2
  std::shared_ptr<roc::BarInterface> mBar2;
  AlfLink mLink;
  std::unique_ptr<LlaSession> mLlaSession;

  static constexpr int DEFAULT_SCA_WAIT_TIME_MS = 3;
};

std::ostream& operator<<(std::ostream& output, const Sca::CommandData& commandData);

} // namespace alf
} // namespace o2

#endif // O2_ALF_INC_SCA_H
