// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file Lla.cxx
/// \brief Implemenetation of an LLA wrapper for ALF
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include "Alf/Lla.h"

namespace o2
{
namespace alf
{

LlaSession::LlaSession(std::string sessionName, roc::SerialId serialId)
  : mSessionName(sessionName),
    mSerialId(serialId)
{
}

void LlaSession::start()
{
  if (!mSession) {
    mParams = lla::SessionParameters::makeParameters(mSessionName, mSerialId);
    mSession = std::make_unique<lla::Session>(mParams);
  }

  if (!mSession->isStarted()) {
    if (!mSession->start()) {
      BOOST_THROW_EXCEPTION(lla::LlaException()
                            << lla::ErrorInfo::Message("Couldn't start session")); // couldn't grab the lock
    }
  }
}

void LlaSession::stop()
{
  mSession->stop();
}

} // namespace alf
} // namespace o2
