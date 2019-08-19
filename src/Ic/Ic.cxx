// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file Ic.cxx
/// \brief Definition of IC operations
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <boost/format.hpp>
#include <chrono>
#include <thread>

#include "AlfException.h"
#include "Logger.h"
#include "Ic/Ic.h"

namespace AliceO2
{
namespace Alf
{

Ic::Ic(AlfLink link) : mBar2(*link.bar2), mLink(link)
{
}

void Ic::reset()
{
}

uint32_t Ic::read()
{
}

void Ic::write()
{
}

void Ic::barWrite(uint32_t index, uint32_t data)
{
  mBar2.writeRegister(index, data);
}

uint32_t Ic::barRead(uint32_t index)
{
  uint32_t read = mBar2.readRegister(index);
  return read;
}

/*std::string Ic::writeSequence(std::vector<std::pair<IcWord, Operation>> words)
{
}*/

} // namespace Alf
} // namespace AliceO2
