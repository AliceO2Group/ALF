
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

/// \file Alf.cxx
/// \brief Definition of the command line tool to run the ALF server
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/asio.hpp>
#include <cstdlib>

#include "AlfServer.h"
#include "Common/Program.h"
#include "DimServices/ServiceNames.h"
#include "Logger.h"
#include "ReadoutCard/CardDescriptor.h"
#include "ReadoutCard/CardFinder.h"
#include "ReadoutCard/ChannelFactory.h"
#include "ReadoutCard/Exception.h"
#include "ReadoutCard/FirmwareChecker.h"

namespace ip = boost::asio::ip;
namespace po = boost::program_options;

namespace o2
{
namespace alf
{

class Alf : public AliceO2::Common::Program
{
 public:
  Alf()
  {
  }

  virtual Description getDescription() override
  {
    return { "ALF", "ALICE Low-level Front-end DIM server", "o2-alf" };
  }

  virtual void addOptions(po::options_description& options) override
  {
    options.add_options()("dim-dns-node",
                          po::value<std::string>(&mOptions.dimDnsNode)->default_value(""),
                          "The DIM DNS node to set the env var if not already set");
    options.add_options()("no-fw-check",
                          po::bool_switch(&mOptions.noFirmwareCheck)->default_value(false),
                          "Disable firmware compatibility check");
    options.add_options()("sequential",
                          po::bool_switch(&mOptions.sequentialRpcs)->default_value(false),
                          "Switch to force DIM RPCs to be executed sequentially");
    options.add_options()("swt-word-size",
                          po::value<std::string>(&mOptions.swtWordSize)->default_value("low"),
                          "Sets the size of SWT word operations (low, medium, high)");
  }

  virtual void run(const po::variables_map&) override
  {
    kDebugLogging = isVerbose();

    Logger::setFacility("ALF");
    Logger::get() << "ALF server starting..." << LogInfoOps << endm;

    if (mOptions.dimDnsNode != "") {
      Logger::get() << "Setting DIM_DNS_NODE from argument." << LogDebugDevel << endm;
      Logger::get() << "DIM_DNS_NODE=" << mOptions.dimDnsNode << LogDebugDevel << endm;
    } else if (const char* dimDnsNode = std::getenv("DIM_DNS_NODE")) {
      Logger::get() << "Picked up DIM_DMS_NODE from the environment." << LogDebugDevel << endm;
      Logger::get() << "DIM_DNS_NODE=" << dimDnsNode << LogDebugDevel << endm;
      mOptions.dimDnsNode = dimDnsNode;
    } else {
      BOOST_THROW_EXCEPTION(AlfException() << ErrorInfo::Message("DIM_DNS_NODE env variable not set, and no relevant argument provided.")); // InfoLogger and errors?
    }

    // Parse the default SWT word size
    SwtWord::Size swtWordSize = SwtWord::Size::Low;
    try {
      swtWordSize = SwtWord::sizeFromString(mOptions.swtWordSize);
    } catch (const ParseException& e) {
      Logger::get() << e.what() << LogWarningOps << endm;
      Logger::get() << "SWT word size defaulting to low" << LogWarningOps << endm;
    }

    std::string alfId = ip::host_name();
    boost::to_upper(alfId);

    Logger::get() << "Starting the DIM Server" << LogInfoDevel << endm;
    DimServer::setDnsNode(mOptions.dimDnsNode.c_str(), 2505);
    DimServer::start(("ALF_" + alfId).c_str());

    AlfServer alfServer = AlfServer(swtWordSize);

    std::vector<roc::CardDescriptor> cardsFound = roc::findCards();
    for (auto const& card : cardsFound) {
      std::vector<AlfLink> links;

      std::shared_ptr<roc::BarInterface> bar;

      // Make the RPC services for every card & link
      if (!mOptions.noFirmwareCheck) {
        try {
          roc::FirmwareChecker().checkFirmwareCompatibility(card.pciAddress);
        } catch (const roc::Exception& e) {
          Logger::get() << e.what() << LogWarningOps << endm;
          continue;
        }
      }

      if (card.cardType == roc::CardType::Cru) {

        Logger::get() << "CRU " << card.serialId << " registered" << LogInfoDevel << endm;
        bar = roc::ChannelFactory().getBar(card.serialId, 2);
        for (int linkId = 0; linkId < kCruNumLinks; linkId++) {
          links.push_back({ alfId, card.serialId, linkId, card.serialId.getEndpoint() * 12 + linkId, bar, roc::CardType::Cru });
        }

      } else if (card.cardType == roc::CardType::Crorc) {
        Logger::get() << "CRORC " << card.serialId << " registered" << LogInfoDevel << endm;
        for (int linkId = 0; linkId < kCrorcNumLinks; linkId++) {
          bar = roc::ChannelFactory().getBar(card.serialId, linkId);
          links.push_back({ alfId, card.serialId, linkId, -1, bar, roc::CardType::Crorc });
        }
      } else {
        Logger::get() << card.serialId << " is not a CRU or a CRORC. Skipping..." << LogWarningDevel << endm;
      }

      if (isVerbose()) {
        for (auto const& link : links) {
          Logger::get() << link.alfId << " " << link.serialId << " " << link.linkId << LogDebugDevel << endm;
        }
      }
      alfServer.makeRpcServers(links, mOptions.sequentialRpcs);
    }

    // main thread
    while (!isSigInt()) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }

 private:
  struct OptionsStruct {
    std::string dimDnsNode = "";
    bool noFirmwareCheck = false;
    bool sequentialRpcs = false;
    std::string swtWordSize = "low";
  } mOptions;
};

} // namespace alf
} // namespace o2

int main(int argc, char** argv)
{
  return o2::alf::Alf().execute(argc, argv);
}
