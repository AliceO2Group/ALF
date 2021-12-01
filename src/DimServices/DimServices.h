
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

/// \file DimServices.h
/// \brief Definition of DIM services related classes
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef O2_ALF_SRC_DIMSERVICES_DIMSERVICES_H
#define O2_ALF_SRC_DIMSERVICES_DIMSERVICES_H

#include <boost/exception/diagnostic_information.hpp>
#include <boost/variant.hpp>
#include <dim/dic.hxx>
#include <dim/dim.hxx>
#include <dim/dis.hxx>
#include <string>

#include <DimRpcParallel/dimrpcparallel.h>

#include "Alf/Exception.h"
#include "Alf/Common.h"
#include "ReadoutCard/Register.h"
#include "Logger.h"

namespace o2
{
namespace alf
{

/// Length of the success/failure prefix that's returned in RPC calls
constexpr size_t PREFIX_LENGTH(8);

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

std::string argumentSeparator();
std::string pairSeparator();
std::string successPrefix();
std::string failurePrefix();
std::string makeSuccessString(const std::string& string);
std::string makeFailureString(const std::string& string);
bool isSuccess(const std::string& str);
bool isFailure(const std::string& str);
std::string stripPrefix(const std::string& str);

class StringRpcServer : public DimRpcParallel
{
 public:
  using Callback = std::function<std::string(const std::string&)>;

  StringRpcServer(const std::string& serviceName, Callback callback, int bank)
    : DimRpcParallel(serviceName.c_str(), "C", "C", bank), mCallback(callback), mServiceName(serviceName)
  {
  }

  StringRpcServer(const StringRpcServer& b) = delete;
  StringRpcServer(StringRpcServer&& b) = delete;

 private:
  void rpcHandler() override;

  Callback mCallback;
  std::string mServiceName;
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
    if (isFailure(str) && kDebugLogging) {
      Logger::get() << "ALF server failure: " << str << LogErrorDevel << endm;
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

} // namespace alf
} // namespace o2
#endif // O2_ALF_SRC_DIMSERVICES_DIMSERVICES_H
