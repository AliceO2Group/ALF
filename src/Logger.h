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


#ifndef ALICEO2_ALF_LOGGER_H_
#define ALICEO2_ALF_LOGGER_H_

#include "InfoLogger/InfoLogger.hxx"

namespace AliceO2
{
namespace Alf
{

constexpr auto endm = AliceO2::InfoLogger::InfoLogger::endm;

inline AliceO2::InfoLogger::InfoLogger& getLogger()
{
  extern AliceO2::InfoLogger::InfoLogger logger;
  return logger;
}

inline AliceO2::InfoLogger::InfoLogger& getWarningLogger()
{
  extern AliceO2::InfoLogger::InfoLogger logger;
  return logger << AliceO2::InfoLogger::InfoLogger::Severity::Warning;
}

inline AliceO2::InfoLogger::InfoLogger& getErrorLogger()
{
  extern AliceO2::InfoLogger::InfoLogger logger;
  return logger << AliceO2::InfoLogger::InfoLogger::Severity::Error;
}

} // namespace AliceO2
} // namespace Alf

#endif // ALICEO2_ALF_COMMON_H_
