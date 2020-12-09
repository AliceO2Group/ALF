// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file ServiceNames.h
/// \brief Definition of DIM service names
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef O2_ALF_SRC_DIMSERVICES_SERVICENAMES_H
#define O2_ALF_SRC_DIMSERVICES_SERVICENAMES_H

#include <string>

#include "Alf/Common.h"

namespace o2
{
namespace alf
{

class ServiceNames
{
 public:
  ServiceNames(AlfLink link)
    : mAlfId(link.alfId), mSerialId(link.serialId), mLink(link.linkId)
  {
  }

  std::string registerRead() const;
  std::string registerWrite() const;
  std::string scaSequence() const;
  std::string swtSequence() const;
  std::string icSequence() const;
  std::string icGbtI2cWrite() const;
  std::string patternPlayer() const;
  std::string registerSequence() const;
  std::string llaSessionStart() const;
  std::string llaSessionStop() const;

 private:
  std::string formatLink(std::string name) const;
  std::string formatCard(std::string name) const;
  std::string mAlfId;
  const roc::SerialId mSerialId;
  const int mLink;
};

} // namespace alf
} // namespace o2

#endif // O2_ALF_SRC_DIMSERVICES_SERVICENAMES_H
