// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file Common.h
/// \brief Definition of common structures for ALF
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef ALICEO2_ALF_COMMON_H_
#define ALICEO2_ALF_COMMON_H_

#include <chrono>
#include "ReadoutCard/BarInterface.h"

namespace AliceO2
{
namespace Alf
{

static constexpr int CRU_NUM_LINKS(24);
static constexpr int CRORC_NUM_LINKS(6);

static constexpr auto BUSY_TIMEOUT = std::chrono::milliseconds(10);
static constexpr auto CHANNEL_BUSY_TIMEOUT = std::chrono::milliseconds(10);

struct AlfLink {
  int alfId;
  int serial;
  int linkId;
  std::shared_ptr<roc::BarInterface> bar2;
};

} // namespace Alf
} // namespace AliceO2

#endif // ALICEO2_ALF_COMMON_H_
