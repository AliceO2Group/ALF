/// \file DimServices.cxx
/// \brief Implementation of DIM services related classes
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <string>

#include "AlfException.h"
#include "boost/algorithm/string/predicate.hpp"
#include "DimServices/DimServices.h"

namespace AliceO2
{
namespace Alf
{

std::vector<char> toCharBuffer(const std::string& str, bool addTerminator)
{
  std::vector<char> buffer(str.begin(), str.end());
  if (addTerminator) {
    buffer.push_back('\0');
  }
  return buffer;
}

/*template <typename DimObject>
void setDataString(const std::string& str, DimObject& dimObject, bool addTerminator)
{
  auto buffer = toCharBuffer(str, addTerminator);
  dimObject.setData(buffer.data(), buffer.size());
}

template <typename DimObject>
void setDataBuffer(std::vector<char>& buffer, DimObject& dimObject)
{
  dimObject.setData(buffer.data(), buffer.size());
}*/

std::string argumentSeparator()
{
  return "\n";
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
    //printf("len=%lu str=%s\n", str.length(), str.c_str());
    BOOST_THROW_EXCEPTION(AlfException() << ErrorInfo::Message("string too short to contain prefix!"));
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

} // namespace AliceO2
} // namespace Alf
