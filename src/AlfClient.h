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

constexpr const uint32_t errHex = 0xffffffff;
constexpr const char* errString = "";

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
    uint32_t converted;
    try {
      toConvert = stripPrefix(getString());
      converted = Util::stringToHex(toConvert);
    } catch (const AlfException& e) {
      getErrorLogger() << "RegisterReadRpc: " << boost::diagnostic_information(e, true) << endm;
      return errHex;
    }

    return converted;
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
    try {
      getString();
    } catch (const AlfException& e) {
      getErrorLogger() << "RegisterWriteRpc: " << boost::diagnostic_information(e, true) << endm;
    }
  }
};

class ScaSequence : DimRpcInfoWrapper
{
 public:
  ScaSequence(const std::string& serviceName)
    : DimRpcInfoWrapper(serviceName)
  {
  }

  std::string write(const std::string& buffer)
  {
    setString(buffer);
    std::string ret;
    try {
      ret = getString();
    } catch (const AlfException& e) {
      getErrorLogger() << "ScaSequence: " << boost::diagnostic_information(e, true) << endm;
      return errString;
    }
    return ret;
  }

  std::string write(const std::vector<std::pair<std::string, std::string>>& sequence)
  {
    std::stringstream buffer;
    for (size_t i = 0; i < sequence.size(); ++i) {
      buffer << sequence[i].first << argumentSeparator() << sequence[i].second;
      if (i + 1 < sequence.size()) {
        buffer << argumentSeparator();
      }
    }
    return write(buffer.str());
  }
};

class SwtSequence : DimRpcInfoWrapper
{
 public:
  SwtSequence(const std::string& serviceName)
    : DimRpcInfoWrapper(serviceName)
  {
  }

  std::string write(const std::string& buffer)
  {
    getWarningLogger() << "BUFFER: " << buffer << endm;
    setString(buffer);
    std::string ret;
    try {
      ret = getString();
    } catch (const AlfException& e) {
      getErrorLogger() << "SwtSequence: " << boost::diagnostic_information(e, true) << endm;
      return errString;
    }
    return ret;
  }

  std::string write(const std::vector<std::pair<std::string, std::string>>& sequence)
  {
    std::stringstream buffer;
    for (size_t i = 0; i < sequence.size(); ++i) {
      buffer << sequence[i].first << pairSeparator() << sequence[i].second;
      if (i + 1 < sequence.size()) {
        buffer << argumentSeparator();
      }
    }
    return write(buffer.str());
  }
};

/* UNUSED FROM NOW ON */

// UNUSED
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
    stream << dnsName << argumentSeparator() << interval;
    for (size_t i = 0; i < addresses.size(); ++i) {
      stream << argumentSeparator() << addresses[i];
    }
    //getLogger() << stream.str() << endm;
    setString(stream.str());
    return getString();
  }
};

// UNUSED
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

// UNUSED
class PublishInfo : DimInfoWrapper //TODO: To be extended with overriden handler for REGS, SCA & SWT ?
{
 public:
  PublishInfo(const std::string& serviceName)
    : DimInfoWrapper(serviceName)
  {
  }
};

// UNUSED
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
    stream << dnsName << argumentSeparator() << interval;
    for (size_t i = 0; i < commandDataPairs.size(); ++i) {
      stream << argumentSeparator() << commandDataPairs[i].command << pairSeparator() << commandDataPairs[i].data;
    }
    //printf("Publish SCA: %s\n", stream.str().c_str());
    setString(stream.str());
    getString();
  }
};

// UNUSED
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

// UNUSED
class PublishSwtSequenceStartRpc : DimRpcInfoWrapper
{
};

// UNUSED
class PublishSwtSequenceStopRpc : DimRpcInfoWrapper
{
};

// UNUSED
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
