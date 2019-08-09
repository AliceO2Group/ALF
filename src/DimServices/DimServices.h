// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file DimServices.h
/// \brief Definition of DIM services related classes
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef ALICEO2_ALF_SRC_DIMSERVICES_DIMSERVICES_H
#define ALICEO2_ALF_SRC_DIMSERVICES_DIMSERVICES_H

#include <boost/exception/diagnostic_information.hpp> 
#include <boost/variant.hpp>
#include <string>

#include "AlfException.h"
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

/// Length of the success/failure prefix that's returned in RPC calls
constexpr size_t PREFIX_LENGTH(8);

/// We use this in a few places because DIM insists on non-const char*
std::vector<char> toCharBuffer(const std::string& string, bool addTerminator = true);

template <typename DimObject>
void setDataString(const std::string& str, DimObject& dimObject, bool addTerminator = true)
{
  auto buffer = toCharBuffer(str, addTerminator);
  dimObject.setData(buffer.data(), buffer.size());
}

template <typename DimObject>
void setDataBuffer(std::vector<char>& buffer, DimObject& dimObject)
{
  dimObject.setData(buffer.data(), buffer.size());
}

/*template <typename DimObject>
void setDataString(const std::string& str, DimObject& dimObject, bool addTerminator = true);
template <typename DimObject>
void setDataBuffer(std::vector<char>& buffer, DimObject& dimObject);*/
std::string argumentSeparator();
std::string successPrefix();
std::string failurePrefix();
std::string makeSuccessString(const std::string& string);
std::string makeFailureString(const std::string& string);
bool isSuccess(const std::string& str);
bool isFailure(const std::string& str);
std::string stripPrefix(const std::string& str);

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

class DimRpcInfoWrapper
{
 public:
   DimRpcInfoWrapper(const std::string& serviceName)
     : mRpcInfo(std::make_unique<DimRpcInfo>(serviceName.c_str(), toCharBuffer("").data()))
   {
   }

   void setString(const std::string& str)
   {
     setDataString(str, getDimRpcInfo());
   }

   std::string getString()
   {
     auto str = std::string(mRpcInfo->getString());
     if (isFailure(str)) {
       BOOST_THROW_EXCEPTION(
           AlfException() << ErrorInfo::Message("ALF server failure: " + str));
     }
     return str;
   }

   template <typename T>
   std::vector<T> getBlob()
   {
     auto data = reinterpret_cast<T*>(mRpcInfo->getData());
     auto size = mRpcInfo->getSize();
     std::vector<T> buffer(data, data + (size / sizeof(T)));
     return buffer;
   }

   DimRpcInfo& getDimRpcInfo() const
   {
     return *mRpcInfo.get();
   }

 private:
   std::unique_ptr<DimRpcInfo> mRpcInfo;
};

class DimInfoWrapper : public DimInfo
{
  public:
    DimInfoWrapper(const std::string &serviceName)
      : DimInfo(serviceName.c_str(), toCharBuffer("").data()),
        mServiceName(serviceName)
    {
    }

    void infoHandler() { //TODO: Requirements?
      getLogger() << "Published value(s) from " << mServiceName << endm;
      getLogger() << getString() << endm;
    }

  private:
    std::string mServiceName;
};

} // namespace AliceO2
} // namespace Alf

#endif // ALICEO2_ALF_SRC_DIMSERVICES_DIMSERVICES_H
