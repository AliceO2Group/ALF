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
/// \brief Includes ReadoutCard logger
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef O2_ALF_LOGGER_H_
#define O2_ALF_LOGGER_H_

#include "ReadoutCard/Logger.h"

typedef AliceO2::roc::Logger Logger;
constexpr auto endm = AliceO2::roc::endm;

#endif // O2_ALF_LOGGER_H_
