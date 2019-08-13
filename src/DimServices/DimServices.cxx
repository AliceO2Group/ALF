// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
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

#include "AlfException.h"
#include "boost/algorithm/string/predicate.hpp"
#include "DimServices/DimServices.h"
#include "Logger.h"

namespace AliceO2
{
namespace Alf
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

void StringRpcServer::rpcHandler()
{
  try {
    auto returnValue = mCallback(std::string(getString()));
    setDataString(makeSuccessString(returnValue), *this);
  } catch (const std::exception& e) {
    getLogger() << InfoLogger::InfoLogger::Error << mServiceName << ": " << boost::diagnostic_information(e, true)
                << endm;
    setDataString(makeFailureString(e.what()), *this);
  }
}

} // namespace Alf
} // namespace AliceO2
