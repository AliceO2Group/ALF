
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

/// \file ServiceNames.cxx
/// \brief Implementation of DIM service names
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch))

#include <boost/format.hpp>

#include "DimServices/ServiceNames.h"

namespace o2
{
namespace alf
{

#define DEFLINKSERVICENAME(_function, _name)  \
  std::string ServiceNames::_function() const \
  {                                           \
    return formatLink(_name);                 \
  }

#define DEFCARDSERVICENAME(_function, _name)  \
  std::string ServiceNames::_function() const \
  {                                           \
    return formatCard(_name);                 \
  }

DEFCARDSERVICENAME(patternPlayer, "PATTERN_PLAYER")
DEFCARDSERVICENAME(llaSessionStart, "LLA_SESSION_START")
DEFCARDSERVICENAME(llaSessionStop, "LLA_SESSION_STOP")
DEFCARDSERVICENAME(registerSequence, "REGISTER_SEQUENCE")

DEFLINKSERVICENAME(registerSequenceLink, "REGISTER_SEQUENCE")
DEFLINKSERVICENAME(scaSequence, "SCA_SEQUENCE")
DEFLINKSERVICENAME(scaMftPsuSequence, "SCA_MFT_PSU_SEQUENCE")
DEFLINKSERVICENAME(swtSequence, "SWT_SEQUENCE")
DEFLINKSERVICENAME(icSequence, "IC_SEQUENCE")
DEFLINKSERVICENAME(icGbtI2cWrite, "IC_GBT_I2C_WRITE")
DEFLINKSERVICENAME(resetCard, "RESET_CARD")

std::string ServiceNames::formatLink(std::string name) const
{
  return ((boost::format("ALF_%1%/SERIAL_%2%/ENDPOINT_%3%/LINK_%4%/%5%") % mAlfId % mSerialId.getSerial() % mSerialId.getEndpoint() % mLink % name)).str();
}

std::string ServiceNames::formatCard(std::string name) const
{
  return ((boost::format("ALF_%1%/SERIAL_%2%/%3%") % mAlfId % mSerialId.getSerial() % name)).str();
}

} // namespace alf
} // namespace o2
