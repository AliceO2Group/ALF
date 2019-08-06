/// \file DimServices.cxx
/// \brief Implementation of DIM services related classes
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <string>

#include "DimServices/DimServices.h"

namespace AliceO2
{
namespace Alf
{

std::vector<char> toCharBuffer(const std::string& string, bool addTerminator)
{
  std::vector<char> buffer(string.begin(), string.end());
  if (addTerminator) {
    buffer.push_back('\0');
  }
  return buffer;
}

template <typename DimObject>
void setDataString(const std::string& string, DimObject& dimObject, bool addTerminator)
{
  auto buffer = toCharBuffer(string, addTerminator);
  dimObject.setData(buffer.data(), buffer.size());
}

/*template <typename DimObject>
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

std::string failPrefix()
{
  return "failure" + argumentSeparator();
}

std::string makeSuccessString(const std::string& string)
{
  return successPrefix() + string;
}

std::string makeFailString(const std::string& string)
{
  return failPrefix() + string;
}

void StringRpcServer::rpcHandler()
{
  try {
    auto returnValue = mCallback(std::string(getString()));
    setDataString(makeSuccessString(returnValue), *this);
  } catch (const std::exception& e) {
    std::cout << mServiceName << ": " << boost::diagnostic_information(e, true)
      << std::endl;
    //getLogger() << InfoLogger::InfoLogger::Error << mServiceName << ": " << boost::diagnostic_information(e, true)
    //  << endm;
    setDataString(makeFailString(e.what()), *this);
  }
}

void serviceAdd(const ServiceDescription& serviceDescription)
{
}

void serviceUpdate(Service& /*service*/)
{
  std::cout << __func__ << " says: Implement me!" << std::endl;
}

void serviceRemove(std::string /*dnsName*/)
{
  std::cout << __func__ << " says: Implement me!" << std::endl;
}


} // namespace AliceO2
} // namespace Alf
