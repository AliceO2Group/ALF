// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file ServiceNames.cxx
/// \brief Implementation of DIM service names
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include <boost/format.hpp>

#include "DimServices/ServiceNames.h"

namespace AliceO2
{
namespace Alf
{

#define DEFLINKSERVICENAME(_function, _name)  \
  std::string ServiceNames::_function() const \
  {                                           \
    return formatLink(_name);                 \
  }

#define DEFCARDSERVICENAME(_function, _name)  \
  std::string ServiceNames::_function() const \
  {                                           \
    return format(_name);                     \
  }

DEFCARDSERVICENAME(registerRead, "REGISTER_READ")
DEFCARDSERVICENAME(registerWrite, "REGISTER_WRITE")
DEFCARDSERVICENAME(patternPlayer, "PATTERN_PLAYER")
DEFCARDSERVICENAME(llaSessionStart, "LLA_SESSION_START")
DEFCARDSERVICENAME(llaSessionStop, "LLA_SESSION_STOP")

DEFLINKSERVICENAME(registerSequence, "REGISTER_SEQUENCE")
DEFLINKSERVICENAME(scaSequence, "SCA_SEQUENCE")
DEFLINKSERVICENAME(swtSequence, "SWT_SEQUENCE")
DEFLINKSERVICENAME(icSequence, "IC_SEQUENCE")
DEFLINKSERVICENAME(icGbtI2cWrite, "IC_GBT_I2C_WRITE")

std::string ServiceNames::formatLink(std::string name) const
{
  return ((boost::format("ALF_%1%/SERIAL_%2%/LINK_%3%/%4%") % mAlfId % mCardSequence % mLink % name)).str();
}

std::string ServiceNames::format(std::string name) const
{
  return ((boost::format("ALF_%1%/SERIAL_%2%/%3%") % mAlfId % mCardSequence % name)).str();
}

} // namespace Alf
} // namespace AliceO2
