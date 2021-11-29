
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

/// \file Lla.cxx
/// \brief Implemenetation of an LLA wrapper for ALF
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include "Alf/Lla.h"

namespace o2
{
namespace alf
{

LlaSession::LlaSession(std::shared_ptr<lla::Session> llaSession)
  : mSession(llaSession)
{
}

LlaSession::LlaSession(std::string sessionName, roc::SerialId serialId)
  : mSessionName(sessionName),
    mSerialId(serialId)
{
}

/* The LlaSession object goes out of scope when the last shared_ptr instance is destroyed (See AlfServer.h).
   Since said destruction may follow an erroneous event and the session might not be explicitly stopped
   it is forcefully stopped it in the destructor */
LlaSession::~LlaSession()
{
  stop();
}

void LlaSession::start()
{
  if (!mSession) {
    mParams = lla::SessionParameters::makeParameters(mSessionName, mSerialId);
    mSession = std::make_shared<lla::Session>(mParams);
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
  if (mSession) {
    mSession->stop();
  }
}

} // namespace alf
} // namespace o2
