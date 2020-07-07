// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
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
  LlaSession(std::string sessionName, int cardId);
  void start();
  void stop();

 private:
  lla::SessionParameters mParams;
  std::unique_ptr<lla::Session> mSession;
  std::string mSessionName;
  int mCardId;
};

} // namespace alf
} // namespace o2

#endif // O2_ALF_INC_LLA_H_
