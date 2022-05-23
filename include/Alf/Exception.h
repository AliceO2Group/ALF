
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

/// \file Exception.h
/// \brief Definition of the ALF exceptions and related functions.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef O2_ALF_INC_EXCEPTION_H_
#define O2_ALF_INC_EXCEPTION_H_

#include <Common/Exceptions.h>

namespace o2
{
namespace alf
{

struct AlfException : AliceO2::Common::Exception {
};
struct ScException : AliceO2::Common::Exception {
};
struct ScaException : AliceO2::Common::Exception {
};
struct ScaMftPsuException : AliceO2::Common::Exception {
};
struct SwtException : AliceO2::Common::Exception {
};
struct ParseException : AliceO2::Common::Exception {
};
struct IcException : AliceO2::Common::Exception {
};
struct PythonException : AliceO2::Common::Exception {
};

namespace ErrorInfo
{
using Message = AliceO2::Common::ErrorInfo::Message;
} // namespace ErrorInfo

} // namespace alf
} // namespace o2

#endif // O2_ALF_INC_EXCEPTION_H_
