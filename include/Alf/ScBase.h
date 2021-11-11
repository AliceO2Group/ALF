
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

/// \file ScBase.h
/// \brief Definition of ALICE Lowlevel Frontend (ALF) SC base class
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef O2_ALF_INC_SCBASE_H
#define O2_ALF_INC_SCBASE_H

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
class ScBase
{
 public:
  /// Internal constructor for the AlfServer
  /// \param link AlfLink holding useful information coming from the AlfServer class
  ScBase(AlfLink link, std::shared_ptr<lla::Session> llaSession);

  /// External constructor
  /// \param cardId The card ID for which to get the SC handle.
  /// \param linkId The link ID to set the channel to (optional).
  ScBase(const roc::Parameters::CardIdType& cardId, int linkId = -1);

  /// External constructor
  /// \param cardId The card ID for which to get the SC handle.
  /// \param linkId The link ID to set the channel to (optional).
  ScBase(std::string cardId, int linkId = -1);

  /// Sets the SC channel
  /// \param gbtChannel The channel to set
  void setChannel(int gbtChannel);

  /// Executes a global SC reset
  void scReset();

  /// Checks if an SC channel has been selected
  /// \throws o2::alf::ScException if no SC channel selected
  void checkChannelSet();

 protected:
  uint32_t barRead(uint32_t index);
  void barWrite(uint32_t index, uint32_t data);

  AlfLink mLink;
  std::shared_ptr<LlaSession> mLlaSession;

 private:
  /// Does the necessary initializations after an object creating
  void init(const roc::Parameters::CardIdType& cardId, int linkId);

  /// Interface for BAR 2
  std::shared_ptr<roc::BarInterface> mBar2;
};

} // namespace alf
} // namespace o2

#endif // O2_ALF_INC_SCBASE_H
