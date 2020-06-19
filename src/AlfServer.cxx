// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file AlfServer.cxx
/// \brief Implementation of ALF server related classes & functions
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <chrono>
#include <iomanip>
#include <thread>

#include "AlfServer.h"
#include "DimServices/ServiceNames.h"
#include "Logger.h"
#include "Util.h"

namespace o2
{
namespace alf
{

AlfServer::AlfServer() : mRpcServers()
{
}

std::string AlfServer::registerRead(const std::string& parameter, std::shared_ptr<roc::BarInterface> bar)
{
  uint32_t address = Util::stringToHex(parameter); // Error from here will get picked up by the StringRpcServer try clause
  //Util::checkAddress(address);

  uint32_t value = bar->readRegister(address / 4);
  return Util::formatValue(value);
}

std::string AlfServer::registerWrite(const std::string& parameter, std::shared_ptr<roc::BarInterface> bar)
{
  std::vector<std::string> params = Util::split(parameter, pairSeparator());

  if (params.size() != 2) {
    BOOST_THROW_EXCEPTION(AlfException() << ErrorInfo::Message("Wrong number of parameters for RPC write call"));
  }

  uint32_t address = Util::stringToHex(params[0]);
  //Util::checkAddress(address);
  uint32_t value = Util::stringToHex(params[1]);

  bar->writeRegister(address / 4, value);
  return "";
}

std::string AlfServer::registerBlobWrite(const std::string& parameter, AlfLink link)
{
  std::vector<std::string> stringPairs = Util::split(parameter, argumentSeparator());
  std::vector<std::vector<uint32_t>> registerPairs = parseStringToRegisterPairs(stringPairs);
  std::stringstream resultBuffer;
  uint32_t value;
  uint32_t address;
  for (const auto& registerPair : registerPairs) {
    address = registerPair.at(0);
    if (registerPair.size() == 1) {
      value = link.bar->readRegister(address / 4);
      resultBuffer << Util::formatValue(value) << "\n";
    } else if (registerPair.size() == 2) {
      value = registerPair.at(1);
      link.bar->writeRegister(address / 4, value);
      resultBuffer << "0"
                   << "\n";
    }
  }
  return resultBuffer.str();
}

std::string AlfServer::scaBlobWrite(const std::string& parameter, AlfLink link)
{
  std::vector<std::string> stringPairs = Util::split(parameter, argumentSeparator());
  std::vector<std::pair<Sca::Operation, Sca::Data>> scaPairs = parseStringToScaPairs(stringPairs);
  Sca sca = Sca(link);
  return sca.writeSequence(scaPairs);
}

std::string AlfServer::swtBlobWrite(const std::string& parameter, AlfLink link)
{

  std::vector<std::string> stringPairs = Util::split(parameter, argumentSeparator());
  std::vector<std::pair<Swt::Operation, Swt::Data>> swtPairs = parseStringToSwtPairs(stringPairs);
  Swt swt = Swt(link);
  return swt.writeSequence(swtPairs);
}

std::string AlfServer::icBlobWrite(const std::string& parameter, AlfLink link)
{

  std::vector<std::string> stringPairs = Util::split(parameter, argumentSeparator());
  std::vector<std::pair<Ic::Operation, Ic::Data>> icPairs = parseStringToIcPairs(stringPairs);
  Ic ic = Ic(link);
  return ic.writeSequence(icPairs);
}

std::string AlfServer::icGbtI2cWrite(const std::string& parameter, AlfLink link)
{
  std::vector<std::string> params = Util::split(parameter, argumentSeparator());

  if (params.size() != 1) {
    BOOST_THROW_EXCEPTION(AlfException() << ErrorInfo::Message("Wrong number of parameters for RPC IC GBT I2C write call"));
  }

  uint32_t value = Util::stringToHex(params[0]);

  Ic ic = Ic(link);
  ic.writeGbtI2c(value);
  return "";
}

std::string AlfServer::patternPlayer(const std::string& parameter, std::shared_ptr<roc::BarInterface> bar2)
{
  std::vector<std::string> parameters = Util::split(parameter, argumentSeparator());
  if (parameters.size() < 11) { // Test that we have enough parameters
    BOOST_THROW_EXCEPTION(AlfException() << ErrorInfo::Message("Wrong number of parameters for the Pattern Player RPC call: " + std::to_string(parameters.size())));
  }

  roc::PatternPlayer::Info info = parseStringToPatternPlayerInfo(parameters);

  roc::PatternPlayer pp = roc::PatternPlayer(bar2);
  pp.play(info);
  return "";
}

std::string AlfServer::llaSessionStart(const std::string& parameter, int cardId)
{
  std::vector<std::string> parameters = Util::split(parameter, pairSeparator());
  if (parameters.size() < 1 || parameters.size() > 2) {
    BOOST_THROW_EXCEPTION(AlfException() << ErrorInfo::Message("Wrong number of parameters for the LLA Session Start RPC call: " + std::to_string(parameters.size())));
  }

  if (mSessions.find(cardId) == mSessions.end()) {
    lla::SessionParameters params = lla::SessionParameters::makeParameters()
                                      .setSessionName(parameters[0])
                                      .setCardId(roc::PciSequenceNumber(cardId));
    mSessions[cardId] = std::make_unique<lla::Session>(params);
  } /*else {
    // TODO: Update session name?
  }*/

  if (parameters.size() == 2) {
    if (!mSessions[cardId]->timedStart(std::stoi(parameters[1]))) {
      BOOST_THROW_EXCEPTION(AlfException() << ErrorInfo::Message("Could not start session for serial " + std::to_string(cardId)));
    }
  } else {
    if (!mSessions[cardId]->start()) {
      BOOST_THROW_EXCEPTION(AlfException() << ErrorInfo::Message("Could not start session for serial " + std::to_string(cardId)));
    }
  }
  return "";
}

std::string AlfServer::llaSessionStop(const std::string& /*parameter*/, int cardId)
{
  if (mSessions.find(cardId) == mSessions.end()) {
    BOOST_THROW_EXCEPTION(AlfException() << ErrorInfo::Message("Session was not started for serial  " + std::to_string(cardId)));
  }

  mSessions[cardId]->stop();
  return "";
}

roc::PatternPlayer::Info AlfServer::parseStringToPatternPlayerInfo(const std::vector<std::string> parameters)
{
  roc::PatternPlayer::Info ppInfo;

  int infoField = 0;
  for (const auto& parameter : parameters) {
    if (parameter.find('#') == std::string::npos) {
      infoField++;
    }
  }

  if (infoField != 11) { // Test that we have enough non-comment parameters
    BOOST_THROW_EXCEPTION(AlfException() << ErrorInfo::Message("Wrong number of non-comment parameters for the Pattern Player RPC call: " + std::to_string(infoField)));
  }

  infoField = 0;
  for (const auto& parameter : parameters) {
    if (parameter.find('#') == std::string::npos) {
      switch (infoField) {
        bool boolValue;
        case 0:
          ppInfo.syncPattern = uint128_t(parameter);
          break;
        case 1:
          ppInfo.resetPattern = uint128_t(parameter);
          break;
        case 2:
          ppInfo.idlePattern = uint128_t(parameter);
          break;
        case 3:
          ppInfo.syncLength = std::stoi(parameter);
          break;
        case 4:
          ppInfo.syncDelay = std::stoi(parameter);
          break;
        case 5:
          ppInfo.resetLength = std::stoi(parameter);
          break;
        case 6:
          ppInfo.resetTriggerSelect = std::stoi(parameter);
          break;
        case 7:
          ppInfo.syncTriggerSelect = std::stoi(parameter);
          break;
        case 8:
          std::istringstream(parameter) >> std::boolalpha >> boolValue;
          ppInfo.syncAtStart = boolValue;
          break;
        case 9:
          std::istringstream(parameter) >> std::boolalpha >> boolValue;
          ppInfo.triggerSync = boolValue;
          break;
        case 10:
          std::istringstream(parameter) >> std::boolalpha >> boolValue;
          ppInfo.triggerReset = boolValue;
          break;
      }
      infoField++;
    }
  }

  return ppInfo;
}

std::vector<uint32_t> AlfServer::stringToRegisterPair(const std::string stringPair)
{
  std::vector<uint32_t> registers;
  auto stringRegisters = Util::split(stringPair, pairSeparator());
  for (const auto& stringRegister : stringRegisters) {
    registers.push_back(Util::stringToHex(stringRegister));
  }
  return registers;
}

std::pair<Sca::Operation, Sca::Data> AlfServer::stringToScaPair(const std::string stringPair)
{
  std::vector<std::string> scaPair = Util::split(stringPair, pairSeparator());
  if (scaPair.size() != 2) {
    BOOST_THROW_EXCEPTION(
      AlfException() << ErrorInfo::Message("SCA command-data pair not formatted correctly"));
  }

  Sca::Data data;
  Sca::Operation operation;

  if (scaPair[scaPair.size() - 1] == "wait") {
    operation = Sca::Operation::Wait;
    try {
      data = std::stoi(scaPair[0]);
    } catch (const std::exception& e) {
      BOOST_THROW_EXCEPTION(SwtException() << ErrorInfo::Message("SCA Wait Time provided cannot be converted to int"));
    }
  } else { // regular sca command
    operation = Sca::Operation::Command;
    Sca::CommandData commandData;
    commandData.command = Util::stringToHex(scaPair[0]);
    commandData.data = Util::stringToHex(scaPair[1]);
    data = commandData;
  }

  return std::make_pair(operation, data);
}

/// Converts a 76-bit hex number string
std::pair<Swt::Operation, Swt::Data> AlfServer::stringToSwtPair(const std::string stringPair)
{
  std::vector<std::string> swtPair = Util::split(stringPair, pairSeparator());
  if (swtPair.size() < 1 || swtPair.size() > 2) {
    BOOST_THROW_EXCEPTION(
      AlfException() << ErrorInfo::Message("SWT word pair not formatted correctly"));
  }

  Swt::Operation operation;

  if (swtPair[swtPair.size() - 1] == "read") {
    operation = Swt::Operation::Read;
  } else if (swtPair[swtPair.size() - 1] == "write") {
    operation = Swt::Operation::Write;
    if (swtPair.size() == 1) {
      BOOST_THROW_EXCEPTION(
        AlfException() << ErrorInfo::Message("Too few arguments for WRITE operation"));
    }
  } else if (swtPair[swtPair.size() - 1] == "reset") {
    operation = Swt::Operation::Reset;
    if (swtPair.size() == 2) {
      BOOST_THROW_EXCEPTION(
        AlfException() << ErrorInfo::Message("Too many arguments for RESET operation"));
    }
  } else {
    BOOST_THROW_EXCEPTION(std::out_of_range("Parameter for SWT operation unkown"));
  }

  Swt::Data data;

  if (operation == Swt::Operation::Write) {
    SwtWord word;
    std::string hexString = swtPair[0];
    std::string leadingHex = "0x";

    std::string::size_type i = hexString.find(leadingHex);
    if (i != std::string::npos) {
      hexString.erase(i, leadingHex.size());
    }

    if (hexString.length() > 19) {
      BOOST_THROW_EXCEPTION(std::out_of_range("Parameter does not fit in 76-bit unsigned int"));
    }

    std::stringstream ss;
    ss << std::setw(19) << std::setfill('0') << hexString;

    word.setHigh(std::stoul(ss.str().substr(0, 3), NULL, 16));
    word.setMed(std::stoul(ss.str().substr(3, 8), NULL, 16));
    word.setLow(std::stoul(ss.str().substr(11, 8), NULL, 16));

    data = word;
  } else if (operation == Swt::Operation::Read && swtPair.size() == 2) {
    try {
      data = std::stoi(swtPair[0]);
    } catch (const std::exception& e) {
      BOOST_THROW_EXCEPTION(SwtException() << ErrorInfo::Message("SWT Read Timeout provided cannot be converted to int"));
    }
  }

  return std::make_pair(operation, data);
}

std::pair<Ic::Operation, Ic::Data> AlfServer::stringToIcPair(const std::string stringPair)
{
  std::vector<std::string> icPair = Util::split(stringPair, pairSeparator());
  if (icPair.size() != 2 && icPair.size() != 3) {
    BOOST_THROW_EXCEPTION(
      AlfException() << ErrorInfo::Message("IC pair not formatted correctly"));
  }

  Ic::Operation icOperation;

  // Parse IC operation
  if (icPair[icPair.size() - 1] == "read") {
    icOperation = Ic::Operation::Read;
    if (icPair.size() == 3) {
      BOOST_THROW_EXCEPTION(
        AlfException() << ErrorInfo::Message("Too many arguments for READ operation"));
    }

  } else if (icPair[icPair.size() - 1] == "write") {
    icOperation = Ic::Operation::Write;
    if (icPair.size() == 2) {
      BOOST_THROW_EXCEPTION(
        AlfException() << ErrorInfo::Message("Too few arguments for WRITE operation"));
    }
  } else {
    BOOST_THROW_EXCEPTION(std::out_of_range("Parameter for IC operation unkown"));
  }

  // Parse IC address
  std::string hexAddress = icPair[0];
  std::string hexData;

  std::string leadingHex = "0x";

  // Validate IC address
  std::string::size_type i = hexAddress.find(leadingHex);
  if (i != std::string::npos) {
    hexAddress.erase(i, leadingHex.size());
  }

  if (hexAddress.length() > 8) {
    BOOST_THROW_EXCEPTION(std::out_of_range("Address parameter does not fit in 16-bit unsigned int"));
  }

  if (icPair.size() == 3) {
    // Parse IC data if present
    hexData = icPair[1];

    // Validate IC data if present
    i = hexData.find(leadingHex);
    if (i != std::string::npos) {
      hexData.erase(i, leadingHex.size());
    }

    if (hexData.length() > 4) {
      BOOST_THROW_EXCEPTION(std::out_of_range("Data parameter does not fit in 8-bit unsigned int"));
    }
  }

  Ic::IcData icData;

  std::stringstream ss;
  ss << std::setw(4) << std::setfill('0') << hexAddress;

  icData.address = std::stoul(ss.str(), NULL, 16);

  ss.str("");
  ss.clear();

  ss << std::setw(2) << std::setfill('0') << hexData;
  icData.data = std::stoul(ss.str(), NULL, 16);

  return std::make_pair(icOperation, icData);
}

std::vector<std::vector<uint32_t>> AlfServer::parseStringToRegisterPairs(std::vector<std::string> stringPairs)
{
  std::vector<std::vector<uint32_t>> pairs;
  for (const auto& stringPair : stringPairs) {
    if (stringPair.find('#') == std::string::npos) {
      pairs.push_back(stringToRegisterPair(stringPair));
    }
  }
  return pairs;
}

std::vector<std::pair<Sca::Operation, Sca::Data>> AlfServer::parseStringToScaPairs(std::vector<std::string> stringPairs)
{
  std::vector<std::pair<Sca::Operation, Sca::Data>> pairs;
  for (const auto& stringPair : stringPairs) {
    if (stringPair.find('#') == std::string::npos) { // =isn't a comment
      pairs.push_back(stringToScaPair(stringPair));
    }
  }
  return pairs;
}

std::vector<std::pair<Swt::Operation, Swt::Data>> AlfServer::parseStringToSwtPairs(std::vector<std::string> stringPairs)
{

  std::vector<std::pair<Swt::Operation, Swt::Data>> pairs;
  for (const auto& stringPair : stringPairs) {
    if (stringPair.find('#') == std::string::npos) {
      pairs.push_back(stringToSwtPair(stringPair));
    }
  }
  return pairs;
}

std::vector<std::pair<Ic::Operation, Ic::Data>> AlfServer::parseStringToIcPairs(std::vector<std::string> stringPairs)
{

  std::vector<std::pair<Ic::Operation, Ic::Data>> pairs;
  for (const auto& stringPair : stringPairs) {
    if (stringPair.find('#') == std::string::npos) {
      pairs.push_back(stringToIcPair(stringPair));
    }
  }
  return pairs;
}

void AlfServer::makeRpcServers(std::vector<AlfLink> links)
{
  for (const auto& link : links) {

    // Function to create RPC server
    auto makeServer = [&](std::string name, auto callback) {
      return std::make_unique<StringRpcServer>(name, callback);
    };

    // Object for generating DNS names for the AlfLink
    ServiceNames names(link);

    // Start the RPC Servers
    auto& servers = mRpcServers[link.cardSequence][link.linkId];
    std::shared_ptr<roc::BarInterface> bar = link.bar;

    if (link.cardType == roc::CardType::Cru) {
      if (link.linkId == 0) { // Register Read / Write services are per card; register them as soon as possible
        // Register Read
        servers.push_back(makeServer(names.registerRead(),
                                     [bar](auto parameter) { return registerRead(parameter, bar); }));
        // Register Write
        servers.push_back(makeServer(names.registerWrite(),
                                     [bar](auto parameter) { return registerWrite(parameter, bar); }));

        // Pattern Player
        servers.push_back(makeServer(names.patternPlayer(),
                                     [bar](auto parameter) { return patternPlayer(parameter, bar); }));

        // LLA Session Start
        servers.push_back(makeServer(names.llaSessionStart(),
                                     [link, this](auto parameter) { return llaSessionStart(parameter, link.cardSequence); }));

        // LLA Session Stop
        servers.push_back(makeServer(names.llaSessionStop(),
                                     [link, this](auto parameter) { return llaSessionStop(parameter, link.cardSequence); }));
      }

      // SCA Sequence
      servers.push_back(makeServer(names.scaSequence(),
                                   [link](auto parameter) { return scaBlobWrite(parameter, link); }));
      // SWT Sequence
      servers.push_back(makeServer(names.swtSequence(),
                                   [link](auto parameter) { return swtBlobWrite(parameter, link); }));
      // IC Sequence
      servers.push_back(makeServer(names.icSequence(),
                                   [link](auto parameter) { return icBlobWrite(parameter, link); }));

      // IC GBT I2C write
      servers.push_back(makeServer(names.icGbtI2cWrite(),
                                   [link](auto parameter) { return icGbtI2cWrite(parameter, link); }));

    } else if (link.cardType == roc::CardType::Crorc) {
      // Register Sequence
      servers.push_back(makeServer(names.registerSequence(),
                                   [link](auto parameter) { return registerBlobWrite(parameter, link); }));
    }
  }
}

} // namespace alf
} // namespace o2
