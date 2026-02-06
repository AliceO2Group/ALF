
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

/// \file DimServices.cxx
/// \brief Implementation of DIM services related classes
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <string>

#include "Alf/Exception.h"
#include "boost/algorithm/string/predicate.hpp"
#include "DimServices/DimServices.h"
#include "Logger.h"

#include <Common/SimpleLog.h>
SimpleLog alfDebugLog;


namespace o2
{
namespace alf
{

/// We use this in a few places because DIM insists on non-const char*
std::vector<char> toCharBuffer(const std::string& str, bool addTerminator)
{
  std::vector<char> buffer(str.begin(), str.end());
  if (addTerminator) {
    buffer.push_back('\0');
  }
  return buffer;
}

std::string argumentSeparator()
{
  return "\n";
}

std::string pairSeparator()
{
  return ",";
}

std::string successPrefix()
{
  return "success" + argumentSeparator();
}

std::string failurePrefix()
{
  return "failure" + argumentSeparator();
}

std::string makeSuccessString(const std::string& str)
{
  return successPrefix() + str;
}

std::string makeFailureString(const std::string& str)
{
  return failurePrefix() + str;
}

bool isSuccess(const std::string& str)
{
  return boost::starts_with(str, successPrefix());
}

bool isFailure(const std::string& str)
{
  return boost::starts_with(str, failurePrefix());
}

std::string stripPrefix(const std::string& str)
{
  if (str.length() < PREFIX_LENGTH) {
    std::stringstream ss;
    ss << "string=" << str << " of size " << str.length() << " too short to contain prefix!" << endm;
    BOOST_THROW_EXCEPTION(AlfException() << ErrorInfo::Message("string too short to contain prefix! " + ss.str()));
  }
  return str.substr(PREFIX_LENGTH);
}

// trim a string in place
inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

void StringRpcServer::rpcHandler()
{
  // build a safe string from DIM input. Parent method getString() is unsafe, not guarateed to be nul-terminated
  std::string inputString;
  if (auto data = getString(); data && getSize() > 0) {
      inputString.assign(data, getSize());
  }

  alfDebugLog.info("Request received on %s : %s",mServiceName.c_str(),inputString.c_str());
  try {
    auto returnValue = mCallback(inputString);
    setDataString(makeSuccessString(returnValue), *this);
    rtrim(returnValue);
    alfDebugLog.info("Request completed: %s", returnValue.c_str());
  } catch (const std::exception& e) {
    if (kDebugLogging) {
      Logger::get() << mServiceName << ": " << e.what() << LogErrorDevel_(5100) << endm;
    }
    setDataString(makeFailureString(e.what()), *this);
    alfDebugLog.error("Request failure: %s", e.what());
  }
}

} // namespace alf
} // namespace o2
