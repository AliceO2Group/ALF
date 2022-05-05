
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

/// \file AlfClient.h
/// \brief Definition of ALF client related classes
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef O2_ALF_ALFCLIENT_H_
#define O2_ALF_ALFCLIENT_H_

#include <boost/format.hpp>
#include <string>

#include "DimServices/DimServices.h"
#include "Util.h"

namespace roc = AliceO2::roc;

namespace o2
{
namespace alf
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
      if (kDebugLogging) {
        Logger::get() << "RegisterReadRpc: " << boost::diagnostic_information(e, true) << LogErrorDevel << endm;
      }
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
    setString((boost::format("0x%x%s0x%x") % registerAddress % pairSeparator() % registerValue).str());
    try {
      getString();
    } catch (const AlfException& e) {
      if (kDebugLogging) {
        Logger::get() << "RegisterWriteRpc: " << boost::diagnostic_information(e, true) << LogErrorDevel << endm;
      }
    }
  }
};

class PatternPlayerRpc : DimRpcInfoWrapper
{
 public:
  PatternPlayerRpc(const std::string& serviceName)
    : DimRpcInfoWrapper(serviceName)
  {
  }
  std::string play(const std::string& buffer)
  {
    setString(buffer);
    std::string ret;
    try {
      ret = getString();
    } catch (const AlfException& e) {
      if (kDebugLogging) {
        Logger::get() << "PatternPlayerRpc: " << boost::diagnostic_information(e, true) << LogErrorDevel << endm;
      }
      return errString;
    }
    return ret;
  }

  std::string play(const std::vector<std::string>& info)
  {
    std::stringstream buffer;
    for (size_t i = 0; i < info.size(); ++i) {
      buffer << info[i];
      if (i + 1 < info.size()) {
        buffer << argumentSeparator();
      }
    }
    return play(buffer.str());
  }
};

class ScaSequenceRpc : DimRpcInfoWrapper
{
 public:
  ScaSequenceRpc(const std::string& serviceName)
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
      if (kDebugLogging) {
        Logger::get() << "ScaSequence: " << boost::diagnostic_information(e, true) << LogErrorDevel << endm;
      }
      return errString;
    }
    return ret;
  }

  std::string write(const std::vector<std::pair<std::string, std::string>>& sequence)
  {
    std::stringstream buffer;
    for (size_t i = 0; i < sequence.size(); ++i) {
      if (sequence[i].first != "") {
        buffer << sequence[i].first << pairSeparator() << sequence[i].second;
      } else {
        buffer << sequence[i].second;
      }
      if (i + 1 < sequence.size()) {
        buffer << argumentSeparator();
      }
    }
    return write(buffer.str());
  }
};

class RegisterSequenceRpc : DimRpcInfoWrapper
{
 public:
  RegisterSequenceRpc(const std::string& serviceName)
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
      if (kDebugLogging) {
        Logger::get() << "RegisterSequence: " << boost::diagnostic_information(e, true) << LogErrorDevel << endm;
      }
      return errString;
    }
    return ret;
  }

  std::string write(const std::vector<std::pair<std::string, std::string>>& sequence)
  {
    std::stringstream buffer;
    for (size_t i = 0; i < sequence.size(); ++i) {
      if (sequence[i].second == "") { // It's a read
        buffer << sequence[i].first;
      } else { //It's a write
        buffer << sequence[i].first << pairSeparator() << sequence[i].second;
      }
      if (i + 1 < sequence.size()) {
        buffer << argumentSeparator();
      }
    }
    return write(buffer.str());
  }
};

class SwtSequenceRpc : DimRpcInfoWrapper
{
 public:
  SwtSequenceRpc(const std::string& serviceName)
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
      if (kDebugLogging) {
        Logger::get() << "SwtSequence: " << boost::diagnostic_information(e, true) << LogErrorDevel << endm;
      }
      return errString;
    }
    return ret;
  }

  std::string write(const std::vector<std::pair<std::string, std::string>>& sequence)
  {
    std::stringstream buffer;
    for (size_t i = 0; i < sequence.size(); ++i) {
      if (sequence[i].first != "") {
        buffer << sequence[i].first << pairSeparator() << sequence[i].second;
      } else {
        buffer << sequence[i].second;
      }
      if (i + 1 < sequence.size()) {
        buffer << argumentSeparator();
      }
    }
    return write(buffer.str());
  }
};

class IcSequenceRpc : DimRpcInfoWrapper
{
 public:
  IcSequenceRpc(const std::string& serviceName)
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
      if (kDebugLogging) {
        Logger::get() << "IcSequence: " << boost::diagnostic_information(e, true) << LogErrorDevel << endm;
      }
      return errString;
    }
    return ret;
  }

  std::string write(const std::vector<std::pair<std::string, std::string>>& sequence)
  {
    std::stringstream buffer;
    for (size_t i = 0; i < sequence.size(); ++i) {
      if (sequence[i].first != "") {
        buffer << sequence[i].first << pairSeparator() << sequence[i].second;
      } else {
        buffer << sequence[i].second;
      }
      if (i + 1 < sequence.size()) {
        buffer << argumentSeparator();
      }
    }
    return write(buffer.str());
  }
};

class IcGbtI2cWriteRpc : DimRpcInfoWrapper
{
 public:
  IcGbtI2cWriteRpc(const std::string& serviceName)
    : DimRpcInfoWrapper(serviceName)
  {
  }

  void write(uint32_t value)
  {
    setString((boost::format("0x%x") % value).str());
    try {
      getString();
    } catch (const AlfException& e) {
      if (kDebugLogging) {
        Logger::get() << "IcGbtI2cWriteRpc: " << boost::diagnostic_information(e, true) << LogErrorDevel << endm;
      }
    }
  }
};

class LlaSessionStartRpc : DimRpcInfoWrapper
{
 public:
  LlaSessionStartRpc(const std::string& serviceName)
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
      if (kDebugLogging) {
        Logger::get() << "LlaSessionStart: " << boost::diagnostic_information(e, true) << LogErrorDevel << endm;
      }
      return errString;
    }
    return ret;
  }

  std::string write(std::string sessionName, int timeOut)
  {
    std::stringstream buffer;
    buffer << sessionName;
    if (timeOut) {
      buffer << "," << timeOut;
    }

    return write(buffer.str());
  }
};

class LlaSessionStopRpc : DimRpcInfoWrapper
{
 public:
  LlaSessionStopRpc(const std::string& serviceName)
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
      if (kDebugLogging) {
        Logger::get() << "LlaSessionStop: " << boost::diagnostic_information(e, true) << LogErrorDevel << endm;
      }
      return errString;
    }
    return ret;
  }
};

class ResetCardRpc : DimRpcInfoWrapper
{
 public:
  ResetCardRpc(const std::string& serviceName)
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
      if (kDebugLogging) {
        Logger::get() << "ResetCard: " << boost::diagnostic_information(e, true) << LogErrorDevel << endm;
      }
      return errString;
    }
    return ret;
  }
};

} // namespace alf
} // namespace o2

#endif // O2_ALF_ALFCLIENT_H_
