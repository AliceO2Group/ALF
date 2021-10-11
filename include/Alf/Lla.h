
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

/// \file Lla.h
/// \brief Definition of an LLA wrapper for ALF
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef O2_ALF_INC_LLA_H_
#define O2_ALF_INC_LLA_H_

#include "Lla/Lla.h"

namespace lla = o2::lla;

namespace o2
{
namespace alf
{

class LlaSession
{
 public:
  LlaSession(std::shared_ptr<lla::Session> llaSession);
  LlaSession(std::string sessionName, roc::SerialId serialId);
  ~LlaSession();
  void start();
  void stop();

 private:
  lla::SessionParameters mParams;
  std::shared_ptr<lla::Session> mSession;
  std::string mSessionName;
  roc::SerialId mSerialId = -1;
};

} // namespace alf
} // namespace o2

#endif // O2_ALF_INC_LLA_H_
