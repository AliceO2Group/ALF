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

#define DEFSERVICENAME(_function, _name)      \
  std::string ServiceNames::_function() const \
  {                                           \
    return format(_name);                     \
  }

DEFSERVICENAME(registerRead, "REGISTER_READ")
DEFSERVICENAME(registerWrite, "REGISTER_WRITE")
DEFSERVICENAME(scaSequence, "SCA_SEQUENCE")
DEFSERVICENAME(swtSequence, "SWT_SEQUENCE")
DEFSERVICENAME(icSequence, "IC_SEQUENCE")
DEFSERVICENAME(icGbtI2cWrite, "IC_GBT_I2C_write")

std::string ServiceNames::format(std::string name) const
{
  if (mAlfId != "-1")
    return ((boost::format("ALF%1%/serial_%2%/link_%3%/%4%") % mAlfId % mSerial % mLink % name)).str();
  else
    return ((boost::format("ALF/serial_%1%/link_%2%/%3%") % mSerial % mLink % name)).str();
}

} // namespace Alf
} // namespace AliceO2
