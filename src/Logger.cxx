// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file Logger.h
/// \brief Definition of InfoLogger functions for ALF
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include "Logger.h"

namespace o2
{
namespace alf
{

Logger& Logger::get()
{
  static Logger instance;
  return instance;
}

AliceO2::InfoLogger::InfoLogger& Logger::log()
{
  return mLogger;
}

AliceO2::InfoLogger::InfoLogger& Logger::warn()
{
  return mLogger << AliceO2::InfoLogger::InfoLogger::Severity::Warning;
}

AliceO2::InfoLogger::InfoLogger& Logger::err()
{
  return mLogger << AliceO2::InfoLogger::InfoLogger::Severity::Error;
}

} // namespace alf
} // namespace o2
