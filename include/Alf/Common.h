
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

/// \file Common.h
/// \brief Definition of common structures for ALF
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef O2_ALF_INC_COMMON_H_
#define O2_ALF_INC_COMMON_H_

#include <chrono>
#include "ReadoutCard/BarInterface.h"
#include "ReadoutCard/CardType.h"

namespace o2
{
namespace alf
{

namespace roc = AliceO2::roc;

static constexpr int kCruNumLinks(12);
static constexpr int kCrorcNumLinks(6);

static constexpr auto BUSY_TIMEOUT = std::chrono::milliseconds(10);
static constexpr auto CHANNEL_BUSY_TIMEOUT = std::chrono::milliseconds(10);

struct AlfLink {
  std::string alfId;
  roc::SerialId serialId = { -1, 0 };
  int linkId;
  int rawLinkId;
  std::shared_ptr<roc::BarInterface> bar;
  roc::CardType::type cardType;
};

} // namespace alf
} // namespace o2

#endif // O2_ALF_INC_COMMON_H_
