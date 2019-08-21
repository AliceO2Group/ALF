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
#include "Swt/Swt.h"
#include "Util.h"

namespace AliceO2
{
namespace Alf
{

AlfServer::AlfServer() : mRpcServers()
{
}

std::string AlfServer::registerRead(const std::string& parameter, std::shared_ptr<roc::BarInterface> bar2)
{
  uint32_t address = Util::stringToHex(parameter); // Error from here will get picked up by the StringRpcServer try clause
  //Util::checkAddress(address);

  uint32_t value = bar2->readRegister(address / 4);
  return Util::formatValue(value);
}

std::string AlfServer::registerWrite(const std::string& parameter, std::shared_ptr<roc::BarInterface> bar2)
{
  std::vector<std::string> params = Util::split(parameter, argumentSeparator());

  if (params.size() != 2) {
    BOOST_THROW_EXCEPTION(AlfException() << ErrorInfo::Message("Wrong number of parameters for RPC write call"));
  }

  uint32_t address = Util::stringToHex(params[0]);
  //Util::checkAddress(address);
  uint32_t value = Util::stringToHex(params[1]);

  bar2->writeRegister(address / 4, value);
  return "";
}

std::string AlfServer::scaBlobWrite(const std::string& parameter, AlfLink link)
{
  std::vector<std::string> stringPairs = Util::split(parameter, argumentSeparator());
  std::vector<Sca::CommandData> commands = parseStringToScaCommands(stringPairs);
  Sca sca = Sca(link);
  return sca.writeSequence(commands);
}

std::string AlfServer::swtBlobWrite(const std::string& parameter, AlfLink link)
{

  std::vector<std::string> stringPairs = Util::split(parameter, argumentSeparator());
  std::vector<std::pair<SwtWord, Swt::Operation>> swtPairs = parseStringToSwtPairs(stringPairs);
  Swt swt = Swt(link);
  return swt.writeSequence(swtPairs);
}

std::string AlfServer::icBlobWrite(const std::string& parameter, AlfLink link)
{

  std::vector<std::string> stringPairs = Util::split(parameter, argumentSeparator());
  std::vector<std::pair<Ic::IcData, Ic::Operation>> icPairs = parseStringToIcPairs(stringPairs);
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

Sca::CommandData AlfServer::stringToScaPair(std::string stringPair)
{
  std::vector<std::string> scaPair = Util::split(stringPair, pairSeparator());
  if (scaPair.size() != 2) {
    BOOST_THROW_EXCEPTION(
      AlfException() << ErrorInfo::Message("SCA command-data pair not formatted correctly"));
  }
  Sca::CommandData commandData;
  commandData.command = Util::stringToHex(scaPair[0]);
  commandData.data = Util::stringToHex(scaPair[1]);
  return commandData;
}

/// Converts a 76-bit hex number string
std::pair<SwtWord, Swt::Operation> AlfServer::stringToSwtPair(const std::string stringPair)
{
  std::vector<std::string> swtPair = Util::split(stringPair, pairSeparator());
  if (swtPair.size() != 1 && swtPair.size() != 2) {
    BOOST_THROW_EXCEPTION(
      AlfException() << ErrorInfo::Message("SWT word pair not formatted correctly"));
  }

  Swt::Operation operation;

  if (swtPair[swtPair.size() - 1] == "read") {
    operation = Swt::Operation::Read;
    if (swtPair.size() == 2) {
      BOOST_THROW_EXCEPTION(
        AlfException() << ErrorInfo::Message("Too many arguments for READ operation"));
    }
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

  SwtWord word;
  if (operation == Swt::Operation::Write) {
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
  }

  return std::make_pair(word, operation);
}

std::pair<Ic::IcData, Ic::Operation> AlfServer::stringToIcPair(const std::string stringPair)
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

  return std::make_pair(icData, icOperation);
}

std::vector<Sca::CommandData> AlfServer::parseStringToScaCommands(std::vector<std::string> stringPairs)
{
  std::vector<Sca::CommandData> pairs;
  for (const auto& stringPair : stringPairs) {
    if (stringPair.find('#') == std::string::npos) { // =isn't a comment
      pairs.push_back(stringToScaPair(stringPair));
    }
  }
  return pairs;
}

std::vector<std::pair<SwtWord, Swt::Operation>> AlfServer::parseStringToSwtPairs(std::vector<std::string> stringPairs)
{

  std::vector<std::pair<SwtWord, Swt::Operation>> pairs;
  for (const auto& stringPair : stringPairs) {
    if (stringPair.find('#') == std::string::npos) {
      pairs.push_back(stringToSwtPair(stringPair));
    }
  }
  return pairs;
}

std::vector<std::pair<Ic::IcData, Ic::Operation>> AlfServer::parseStringToIcPairs(std::vector<std::string> stringPairs)
{

  std::vector<std::pair<Ic::IcData, Ic::Operation>> pairs;
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
    auto& servers = mRpcServers[link.serial][link.linkId];
    std::shared_ptr<roc::BarInterface> bar2 = link.bar2;

    // Register Read
    servers.push_back(makeServer(names.registerRead(),
                                 [bar2](auto parameter) { return registerRead(parameter, bar2); }));
    // Register Write
    servers.push_back(makeServer(names.registerWrite(),
                                 [bar2](auto parameter) { return registerWrite(parameter, bar2); }));

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
  }
}

} // namespace Alf
} // namespace AliceO2
