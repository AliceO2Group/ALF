// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file AlfClient.h
/// \brief Definition of ALF client related classes
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef ALICEO2_ALF_ALFCLIENT_H_
#define ALICEO2_ALF_ALFCLIENT_H_

#include <boost/format.hpp>
#include <string>

#include "DimServices/DimServices.h"
#include "Util.h"


namespace roc = AliceO2::roc;

namespace AliceO2
{
namespace Alf
{

class RegisterReadRpc : DimRpcInfoWrapper
{
 public:
  RegisterReadRpc(const std::string& serviceName)
    : DimRpcInfoWrapper(serviceName)
  {
  }

  uint32_t readRegister(uint64_t registerAddress)
  {
    setString((boost::format("0x%x") % registerAddress).str());
    std::string toConvert;
    try {
      toConvert = stripPrefix(getString());
    } catch (const AlfException& e) {
      getErrorLogger() << "RegisterReadRpc: " << boost::diagnostic_information(e, true) << endm;
      return 0xffffffff; //TODO: Better value here?
    }

    return Util::stringToHex(toConvert);
  }
};

class RegisterWriteRpc : DimRpcInfoWrapper
{
 public:
  RegisterWriteRpc(const std::string& serviceName)
    : DimRpcInfoWrapper(serviceName)
  {
  }

  void writeRegister(uint64_t registerAddress, uint32_t registerValue)
  {
    setString((boost::format("0x%x%s0x%x") % registerAddress % argumentSeparator() % registerValue).str());
    getString();
  }
};

class ScaWriteSequence : DimRpcInfoWrapper
{
 public:
  ScaWriteSequence(const std::string& serviceName)
    : DimRpcInfoWrapper(serviceName)
  {
  }

  std::string write(const std::string& buffer)
  {
    setString(buffer);
    return getString();
  }

  std::string write(const std::vector<std::pair<uint32_t, uint32_t>>& sequence)
  {
    std::stringstream buffer;
    for (size_t i = 0; i < sequence.size(); ++i) {
      buffer << sequence[i].first << Sca::pairSeparator() << sequence[i].second;
      if (i + 1 < sequence.size()) {
        buffer << argumentSeparator();
      }
    }
    return write(buffer.str());
  }
};

class SwtWriteSequence : DimRpcInfoWrapper
{
};

class PublishRegistersStartRpc : DimRpcInfoWrapper
{
 public:
  PublishRegistersStartRpc(const std::string& serviceName)
    : DimRpcInfoWrapper(serviceName)
  {
  }

  std::string publish(std::string dnsName, double interval, std::vector<size_t> addresses)
  {
    std::ostringstream stream;
    auto sep = argumentSeparator();
    stream << dnsName << sep << interval;
    for (size_t i = 0; i < addresses.size(); ++i) {
      stream << sep << addresses[i];
    }
    //getLogger() << stream.str() << endm;
    setString(stream.str());
    return getString();
  }
};

class PublishRegistersStopRpc : DimRpcInfoWrapper
{
 public:
  PublishRegistersStopRpc(const std::string& serviceName)
    : DimRpcInfoWrapper(serviceName)
  {
  }

  void stop(std::string dnsName)
  {
    setString(dnsName);
    getString();
  }
};

class PublishInfo : DimInfoWrapper //TODO: To be extended with overriden handler for REGS, SCA & SWT ?
{
 public:
  PublishInfo(const std::string& serviceName)
    : DimInfoWrapper(serviceName)
  {
  }
};

class PublishScaSequenceStartRpc : DimRpcInfoWrapper
{
 public:
  PublishScaSequenceStartRpc(const std::string& serviceName)
    : DimRpcInfoWrapper(serviceName)
  {
  }

  void publish(std::string dnsName, double interval, const std::vector<Sca::CommandData>& commandDataPairs)
  {
    std::ostringstream stream;
    auto sep = argumentSeparator();
    stream << dnsName << sep << interval;
    for (size_t i = 0; i < commandDataPairs.size(); ++i) {
      stream << sep << commandDataPairs[i].command << Sca::pairSeparator() << commandDataPairs[i].data;
    }
    //printf("Publish SCA: %s\n", stream.str().c_str());
    setString(stream.str());
    getString();
  }
};

class PublishScaSequenceStopRpc : DimRpcInfoWrapper
{
 public:
  PublishScaSequenceStopRpc(const std::string& serviceName)
    : DimRpcInfoWrapper(serviceName)
  {
  }

  void stop(std::string dnsName)
  {
    setString(dnsName);
    getString();
  }
};

class PublishSwtSequenceStartRpc : DimRpcInfoWrapper
{
};

class PublishSwtSequenceStopRpc : DimRpcInfoWrapper
{
};

/*class ScaReadRpc: DimRpcInfoWrapper //TODO: Does this stay??
{
  public:
    ScaReadRpc(const std::string& serviceName)
      : DimRpcInfoWrapper(serviceName)
    {
    }

    std::string read()
    {
      setString("");
      return stripPrefix(getString());
    }
};*/

} // namespace Alf
} // namespace AliceO2

#endif // ALICEO2_ALF_ALFCLIENT_H_
