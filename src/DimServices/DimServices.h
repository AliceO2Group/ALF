/// \file DimServices.h
/// \brief Definition of DIM services related classes
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef ALICEO2_ALF_SRC_DIMSERVICES_DIMSERVICES_H
#define ALICEO2_ALF_SRC_DIMSERVICES_DIMSERVICES_H

#include <boost/exception/diagnostic_information.hpp> 
#include <boost/variant.hpp>
#include <iostream>
#include <string>

#include "Common.h"
#include "ReadoutCard/Register.h"
#include "Sca/Sca.h"
#include "Swt/SwtWord.h"

#include <dim/dic.hxx>
#include <dim/dim.hxx>
#include <dim/dis.hxx>

namespace AliceO2
{
namespace Alf
{

/// We use this in a few places because DIM insists on non-const char*
std::vector<char> toCharBuffer(const std::string& string, bool addTerminator = true);
template <typename DimObject>
void setDataString(const std::string& string, DimObject& dimObject, bool addTerminator = true);
template <typename DimObject>
void setDataBuffer(std::vector<char>& buffer, DimObject& dimObject);
std::string argumentSeparator();
std::string successPrefix();
std::string failPrefix();
std::string makeSuccessString(const std::string& string);
std::string makeFailString(const std::string& string);

class StringRpcServer: public DimRpc
{
  public:
    using Callback = std::function<std::string(const std::string&)>;

    StringRpcServer(const std::string& serviceName, Callback callback)
      : DimRpc(serviceName.c_str(), "C", "C"), mCallback(callback), mServiceName(serviceName)
    {
    }

    StringRpcServer(const StringRpcServer& b) = delete;
    StringRpcServer(StringRpcServer&& b) = delete;

  private:
    void rpcHandler() override;

    Callback mCallback;
    std::string mServiceName;
};

/// Struct describing a DIM publishing service
struct ServiceDescription //Should this be further abstracted?
{
  /// Struct for register read service
  struct Register
  {
    std::vector<uintptr_t> addresses;
  };

  /// Struct for SCA sequence service
  struct ScaSequence
  {
    std::vector<Sca::CommandData> commandDataPairs;
  };

  /// Struct for SWT sequence service
  struct SwtSequence
  {
    std::vector<SwtWord> swtWords;
  };

  std::string dnsName;
  std::chrono::milliseconds interval;
  boost::variant<Register, ScaSequence, SwtSequence> type;
  AlfLink link;
};

/// Struct for DIM publishing service data
struct Service
{
 public:
   void advanceUpdateTime()
   {
     nextUpdate = nextUpdate + description.interval;
   }

   ServiceDescription description;
   std::chrono::steady_clock::time_point nextUpdate;
   std::unique_ptr<DimService> dimService;
   std::vector<char> buffer; ///< Needed for DIM
};

void serviceAdd(const ServiceDescription& serviceDescription);
void serviceUpdate(Service& service);
void serviceRemove(std::string dnsName);


} // namespace AliceO2
} // namespace Alf

#endif // ALICEO2_ALF_SRC_DIMSERVICES_DIMSERVICES_H
