
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

#include "ReadoutCard/ChannelFactory.h"
#include "ReadoutCard/Exception.h"

namespace o2
{
namespace alf
{

AlfServer::AlfServer(SwtWord::Size swtWordSize) : mRpcServers(), mSwtWordSize(swtWordSize)
{
}

std::string AlfServer::registerBlobWrite(const std::string& parameter, std::shared_ptr<roc::BarInterface> bar, bool isCru)
{
  std::vector<std::string> stringPairs = Util::split(parameter, argumentSeparator());
  std::vector<std::vector<uint32_t>> registerPairs = parseStringToRegisterPairs(stringPairs);
  std::stringstream resultBuffer;
  uint32_t value;
  uint32_t address;
  for (const auto& registerPair : registerPairs) {
    address = registerPair.at(0);
    // If it's a CRU, check address range
    if (isCru && (address < 0x00c00000 || address > 0x00cfffff)) {
      resultBuffer << "Illegal address 0x" << std::hex << address
                   << ", allowed: [0x00c0_0000-0x00cf_ffff]"
                   << "\n";
      BOOST_THROW_EXCEPTION(AlfException() << ErrorInfo::Message(resultBuffer.str()));
    }

    if (registerPair.size() == 1) {
      value = bar->readRegister(address / 4);
      resultBuffer << Util::formatValue(value) << "\n";
    } else if (registerPair.size() == 2) {
      value = registerPair.at(1);
      bar->writeRegister(address / 4, value);
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
  Sca sca = Sca(link, mSessions[link.serialId]);

  bool lock = false;
  // Check if the operation should be locked
  if (scaPairs[0].first == Sca::Operation::Lock) {
    scaPairs.erase(scaPairs.begin());
    lock = true;
  }
  return sca.writeSequence(scaPairs, lock);
}

std::string AlfServer::scaMftPsuBlobWrite(const std::string& parameter, AlfLink link)
{
  std::vector<std::string> stringPairs = Util::split(parameter, argumentSeparator());
  std::vector<std::pair<Sca::Operation, Sca::Data>> scaPairs = parseStringToScaPairs(stringPairs);
  ScaMftPsu sca = ScaMftPsu(link, mSessions[link.serialId]);

  bool lock = false;
  // Check if the operation should be locked
  if (scaPairs[0].first == Sca::Operation::Lock) {
    scaPairs.erase(scaPairs.begin());
    lock = true;
  }
  return sca.writeSequence(scaPairs, lock);
}

std::string AlfServer::swtBlobWrite(const std::string& parameter, AlfLink link)
{

  std::vector<std::string> stringPairs = Util::split(parameter, argumentSeparator());
  std::vector<std::pair<Swt::Operation, Swt::Data>> swtPairs = parseStringToSwtPairs(stringPairs, mSwtWordSize);
  Swt swt = Swt(link, mSessions[link.serialId], mSwtWordSize);

  bool lock = false;
  // Check if the operation should be locked
  if (swtPairs[0].first == Swt::Operation::Lock) {
    swtPairs.erase(swtPairs.begin());
    lock = true;
  }
  return swt.writeSequence(swtPairs, lock);
}

std::string AlfServer::icBlobWrite(const std::string& parameter, AlfLink link)
{

  std::vector<std::string> stringPairs = Util::split(parameter, argumentSeparator());
  std::vector<std::pair<Ic::Operation, Ic::Data>> icPairs = parseStringToIcPairs(stringPairs);
  Ic ic = Ic(link, mSessions[link.serialId]);

  bool lock = false;
  // Check if the operation should be locked
  if (icPairs[0].first == Ic::Operation::Lock) {
    icPairs.erase(icPairs.begin());
    lock = true;
  }
  return ic.writeSequence(icPairs, lock);
}

std::string AlfServer::icGbtI2cWrite(const std::string& parameter, AlfLink link)
{
  std::vector<std::string> params = Util::split(parameter, argumentSeparator());

  if (params.size() != 1) {
    BOOST_THROW_EXCEPTION(AlfException() << ErrorInfo::Message("Wrong number of parameters for RPC IC GBT I2C write call"));
  }

  uint32_t value = Util::stringToHex(params[0]);

  Ic ic = Ic(link, mSessions[link.serialId]);
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

std::string AlfServer::llaSessionStart(const std::string& parameter, roc::SerialId serialId)
{
  std::vector<std::string> parameters = Util::split(parameter, pairSeparator());
  if (parameters.size() < 1 || parameters.size() > 2) {
    BOOST_THROW_EXCEPTION(AlfException() << ErrorInfo::Message("Wrong number of parameters for the LLA Session Start RPC call: " + std::to_string(parameters.size())));
  }

  if (mSessions.find(serialId) == mSessions.end()) {
    BOOST_THROW_EXCEPTION(AlfException() << ErrorInfo::Message("Session not initialized for serial  " + serialId.toString()));
  } /*else {
    // TODO: Update session name?
  }*/

  if (parameters.size() == 2) {
    if (!mSessions[serialId]->timedStart(std::stoi(parameters[1]))) {
      BOOST_THROW_EXCEPTION(AlfException() << ErrorInfo::Message("Could not start session for serial " + serialId.toString()));
    }
  } else {
    if (!mSessions[serialId]->start()) {
      BOOST_THROW_EXCEPTION(AlfException() << ErrorInfo::Message("Could not start session for serial " + serialId.toString()));
    }
  }
  return "";
}

std::string AlfServer::llaSessionStop(const std::string& /*parameter*/, roc::SerialId serialId)
{
  if (mSessions.find(serialId) == mSessions.end()) {
    BOOST_THROW_EXCEPTION(AlfException() << ErrorInfo::Message("Session was not started for serial  " + serialId.toString()));
  }

  mSessions[serialId]->stop();
  return "";
}

std::string AlfServer::resetCard(const std::string& /*parameter*/, AlfLink link)
{
  // Reset the CRORC DMA channel
  auto params = roc::Parameters::makeParameters(link.serialId, link.linkId);
  params.setBufferParameters(o2::roc::buffer_parameters::Null());
  params.setFirmwareCheckEnabled(false);
  std::shared_ptr<roc::DmaChannelInterface> dmaChannel;
  try {
    dmaChannel = roc::ChannelFactory().getDmaChannel(params);
  } catch (const roc::LockException& e) {
    BOOST_THROW_EXCEPTION(
        AlfException() << ErrorInfo::Message("Another process is holding the channel lock (cannot reset)"));
  }
  dmaChannel->resetChannel(roc::ResetLevel::InternalSiu);

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

  Sca::Data data;
  Sca::Operation operation;

  if (scaPair.size() < 1 || scaPair.size() > 2) {
    BOOST_THROW_EXCEPTION(
      AlfException() << ErrorInfo::Message("SCA command-data pair not formatted correctly"));
  }

  if (scaPair[scaPair.size() - 1] == "lock") {
    operation = Sca::Operation::Lock;
    if (scaPair.size() != 1) {
      BOOST_THROW_EXCEPTION(
        AlfException() << ErrorInfo::Message("Too many arguments for LOCK operation"));
    }
  } else if (scaPair[scaPair.size() - 1] == "wait") {
    operation = Sca::Operation::Wait;
    if (scaPair.size() != 2) {
      BOOST_THROW_EXCEPTION(
        AlfException() << ErrorInfo::Message("Too few arguments for WAIT operation"));
    }
    try {
      data = std::stoi(scaPair[0]);
    } catch (const std::exception& e) {
      BOOST_THROW_EXCEPTION(SwtException() << ErrorInfo::Message("SCA Wait Time provided cannot be converted to int"));
    }
  } else if (scaPair[scaPair.size() - 1] == "svl_reset") {
    operation = Sca::Operation::SVLReset;
    if (scaPair.size() != 1) {
      BOOST_THROW_EXCEPTION(
        AlfException() << ErrorInfo::Message("Too many arguments for SVL RESET operation"));
    }
  } else if (scaPair[scaPair.size() - 1] == "svl_connect") {
    operation = Sca::Operation::SVLConnect;
    if (scaPair.size() != 1) {
      BOOST_THROW_EXCEPTION(
        AlfException() << ErrorInfo::Message("Too many arguments for SVL CONNECT operation"));
    }
  } else if (scaPair[scaPair.size() - 1] == "sc_reset") {
    operation = Sca::Operation::SCReset;
    if (scaPair.size() != 1) {
      BOOST_THROW_EXCEPTION(
        AlfException() << ErrorInfo::Message("Too many arguments for SC RESET operation"));
    }
  } else if (scaPair[scaPair.size() - 1] == "master") {
    operation = Sca::Operation::Master;
    if (scaPair.size() != 1) {
      BOOST_THROW_EXCEPTION(
        AlfException() << ErrorInfo::Message("Too many arguments for MASTER operation"));
    }
  } else if (scaPair[scaPair.size() - 1] == "slave") {
    operation = Sca::Operation::Slave;
    if (scaPair.size() != 1) {
      BOOST_THROW_EXCEPTION(
        AlfException() << ErrorInfo::Message("Too many arguments for SLAVE operation"));
    }
  } else { // regular sca command
    operation = Sca::Operation::Command;
    if (scaPair.size() != 2) {
      BOOST_THROW_EXCEPTION(
        AlfException() << ErrorInfo::Message("Too few arguments for SCA command-data pair"));
    }
    Sca::CommandData commandData;
    commandData.command = Util::stringToHex(scaPair[0]);
    commandData.data = Util::stringToHex(scaPair[1]);
    data = commandData;
  }

  return std::make_pair(operation, data);
}

/// Converts a 76-bit hex number string
std::pair<Swt::Operation, Swt::Data> AlfServer::stringToSwtPair(const std::string stringPair, const SwtWord::Size swtWordSize)
{
  std::vector<std::string> swtPair = Util::split(stringPair, pairSeparator());
  if (swtPair.size() < 1 || swtPair.size() > 2) {
    BOOST_THROW_EXCEPTION(
      AlfException() << ErrorInfo::Message("SWT word pair not formatted correctly"));
  }

  Swt::Operation operation;

  if (swtPair[swtPair.size() - 1] == "lock") {
    operation = Swt::Operation::Lock;
    if (swtPair.size() == 2) {
      BOOST_THROW_EXCEPTION(
        AlfException() << ErrorInfo::Message("Too many arguments for LOCK operation"));
    }
  } else if (swtPair[swtPair.size() - 1] == "read") {
    operation = Swt::Operation::Read;
  } else if (swtPair[swtPair.size() - 1] == "write") {
    operation = Swt::Operation::Write;
    if (swtPair.size() == 1) {
      BOOST_THROW_EXCEPTION(
        AlfException() << ErrorInfo::Message("Too few arguments for WRITE operation"));
    }
  } else if (swtPair[swtPair.size() - 1] == "sc_reset") {
    operation = Swt::Operation::SCReset;
    if (swtPair.size() == 2) {
      BOOST_THROW_EXCEPTION(
        AlfException() << ErrorInfo::Message("Too many arguments for SC RESET operation"));
    }
  } else if (swtPair[swtPair.size() - 1] == "wait") {
    operation = Swt::Operation::Wait;
  } else {
    BOOST_THROW_EXCEPTION(std::out_of_range("Parameter for SWT operation unkown"));
  }

  Swt::Data data;

  if (operation == Swt::Operation::Write) {
    SwtWord word;
    word.setSize(swtWordSize);
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
  } else if (operation == Swt::Operation::Wait && swtPair.size() == 2) {
    try {
      data = std::stoi(swtPair[0]);
    } catch (const std::exception& e) {
      BOOST_THROW_EXCEPTION(SwtException() << ErrorInfo::Message("SWT WaitTime provided cannot be converted to int"));
    }
  }

  return std::make_pair(operation, data);
}

std::pair<Ic::Operation, Ic::Data> AlfServer::stringToIcPair(const std::string stringPair)
{
  std::vector<std::string> icPair = Util::split(stringPair, pairSeparator());
  if (icPair.size() < 1 || icPair.size() > 3) {
    BOOST_THROW_EXCEPTION(
      AlfException() << ErrorInfo::Message("IC pair not formatted correctly"));
  }

  Ic::Operation icOperation;
  Ic::IcData icData;

  // Parse IC operation
  if (icPair[icPair.size() - 1] == "lock") {
    icOperation = Ic::Operation::Lock;
    if (icPair.size() > 1) {
      BOOST_THROW_EXCEPTION(
        AlfException() << ErrorInfo::Message("Too many arguments for LOCK operation"));
    }
    return std::make_pair(icOperation, icData); // no data to parse, return immediately
  } else if (icPair[icPair.size() - 1] == "read") {
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

std::vector<std::pair<Swt::Operation, Swt::Data>> AlfServer::parseStringToSwtPairs(std::vector<std::string> stringPairs, const SwtWord::Size swtWordSize)
{

  std::vector<std::pair<Swt::Operation, Swt::Data>> pairs;
  for (const auto& stringPair : stringPairs) {
    if (stringPair.find('#') == std::string::npos) {
      pairs.push_back(stringToSwtPair(stringPair, swtWordSize));
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

void AlfServer::makeRpcServers(std::vector<AlfLink> links, bool sequentialRpcs)
{
  for (auto& link : links) {
    // Set a unique parallel dim rpc bank for every link
    int parallelDimRpcBank = sequentialRpcs ? 0 : (link.serialId.getSerial() * 100) + link.rawLinkId;

    // Function to create RPC server
    auto makeServer = [&](std::string name, auto callback) {
      return std::make_unique<StringRpcServer>(name, callback, parallelDimRpcBank);
    };

    // Object for generating DNS names for the AlfLink
    ServiceNames names(link);

    // Start the RPC Servers
    auto& servers = mRpcServers[link.serialId][link.linkId];
    std::shared_ptr<roc::BarInterface> bar = link.bar;

    if (link.cardType == roc::CardType::Cru) {
      lla::SessionParameters params = lla::SessionParameters::makeParameters()
                                        .setSessionName("ALF")
                                        .setCardId(link.serialId);
      mSessions[link.serialId] = std::make_shared<lla::Session>(params);

      if (ScaMftPsu::isAnMftPsuLink(link)) {
        // SCA MFT PSU Sequence
        servers.push_back(makeServer(names.scaMftPsuSequence(),
                                     [link, this](auto parameter) { return scaMftPsuBlobWrite(parameter, link); }));
        continue;
      }

      if (link.linkId == 0 && link.serialId.getEndpoint() == 0) { // Services per card

        // Register Sequence
        servers.push_back(makeServer(names.registerSequence(),
                                     [bar](auto parameter) { return registerBlobWrite(parameter, bar, true); }));
        // Pattern Player
        servers.push_back(makeServer(names.patternPlayer(),
                                     [bar](auto parameter) { return patternPlayer(parameter, bar); }));

        // LLA Session Start
        servers.push_back(makeServer(names.llaSessionStart(),
                                     [link, this](auto parameter) { return llaSessionStart(parameter, link.serialId); }));

        // LLA Session Stop
        servers.push_back(makeServer(names.llaSessionStop(),
                                     [link, this](auto parameter) { return llaSessionStop(parameter, link.serialId); }));
      }

      // SCA Sequence
      servers.push_back(makeServer(names.scaSequence(),
                                   [link, this](auto parameter) { return scaBlobWrite(parameter, link); }));
      // SWT Sequence
      servers.push_back(makeServer(names.swtSequence(),
                                   [link, this](auto parameter) { return swtBlobWrite(parameter, link); }));
      // IC Sequence
      servers.push_back(makeServer(names.icSequence(),
                                   [link, this](auto parameter) { return icBlobWrite(parameter, link); }));

      // IC GBT I2C write
      servers.push_back(makeServer(names.icGbtI2cWrite(),
                                   [link, this](auto parameter) { return icGbtI2cWrite(parameter, link); }));

    } else if (link.cardType == roc::CardType::Crorc) {
      // Register Sequence
      servers.push_back(makeServer(names.registerSequenceLink(),
                                   [bar](auto parameter) { return registerBlobWrite(parameter, bar); }));
      servers.push_back(makeServer(names.resetCard(),
                                   [link, this](auto parameter) { return resetCard(parameter, link); }));
    }
  }
}

} // namespace alf
} // namespace o2
