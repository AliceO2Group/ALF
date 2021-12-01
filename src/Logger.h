
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

/// \file Logger.h
/// \brief Includes ReadoutCard logger
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef O2_ALF_LOGGER_H_
#define O2_ALF_LOGGER_H_

#include "ReadoutCard/Logger.h"

typedef AliceO2::roc::Logger Logger;
constexpr auto endm = AliceO2::roc::endm;

extern bool kDebugLogging;

#endif // O2_ALF_LOGGER_H_
