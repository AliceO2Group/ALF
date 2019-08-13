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

#ifndef ALICEO2_ALF_SRC_SCA_SCA_H
#define ALICEO2_ALF_SRC_SCA_SCA_H

#include <string>
#include <map>

#include "Common.h"
#include "ReadoutCard/CardType.h"
#include "ReadoutCard/RegisterReadWriteInterface.h"

namespace roc = AliceO2::roc;

namespace AliceO2
{
namespace Alf
{

/// Class for interfacing with the C-RORC's(?) and CRU's Slow-Control Adapter (SCA)
class Sca
{
 public:
  struct CommandData {
    uint32_t command;
    uint32_t data;
  };

  struct ReadResult {
    uint32_t command;
    uint32_t data;
  };

  /// \param bar2 SCA is on BAR 2
  /// \param link Needed to get offset for SCA registers
  Sca(AlfLink link);

  void initialize();
  ReadResult gpioRead();
  ReadResult gpioWrite(uint32_t data);
  ReadResult read();
  void write(uint32_t command, uint32_t data);
  void write(CommandData commandData)
  {
    write(commandData.command, commandData.data);
  };

  std::string writeSequence(const std::vector<CommandData>& commands);

 private:
  uint32_t barRead(uint32_t index);
  void barWrite(uint32_t index, uint32_t data);
  void checkError(uint32_t command);
  void executeCommand();
  void gpioEnable();
  void init();
  bool isChannelBusy(uint32_t command);
  void waitOnBusyClear();

  /// Interface for BAR 2
  roc::RegisterReadWriteInterface& mBar2;

  AlfLink mLink;
};

} // namespace Alf
} // namespace AliceO2

#endif // ALICEO2_ALF_SRC_SCA_SCA_H
