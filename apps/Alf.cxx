
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

#include <Common/SimpleLog.h>
extern SimpleLog alfDebugLog;

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
    options.add_options()("dim-log-file",
                          po::value<std::string>(&mOptions.dimLogFileConfig)->default_value(""),
                          "Sets the log file to track DIM callbacks: filePath,maxSize,rotateCount");
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
    Logger::get() << "ALF server starting..." << LogInfoOps_(5000) << endm;

    if (mOptions.dimLogFileConfig!="") {
      std::string path;
      unsigned long maxBytes = 0;    // default if missing
      unsigned int  maxFiles = 0;   // default if missing

      size_t start = 0;
      size_t comma = mOptions.dimLogFileConfig.find(',');

      // ---- first (mandatory): path ----
      if (comma == std::string::npos) {
        path = mOptions.dimLogFileConfig;
      } else {
	path = mOptions.dimLogFileConfig.substr(0, comma);
	start = comma + 1;

	// ---- second (optional): maxBytes ----
	comma = mOptions.dimLogFileConfig.find(',', start);
	if (comma == std::string::npos) {
	  if (start < mOptions.dimLogFileConfig.size())
	    maxBytes = std::stoul(mOptions.dimLogFileConfig.substr(start));
	} else {
	  maxBytes = std::stoul(mOptions.dimLogFileConfig.substr(start, comma - start));
	  start = comma + 1;

	  // ---- third (optional): maxFiles ----
	  if (start < mOptions.dimLogFileConfig.size())
	    maxFiles = static_cast<unsigned int>(std::stoul(mOptions.dimLogFileConfig.substr(start)));
	}
      }
      const char* debugLogFile = path.c_str();
      if (path == "stdout") debugLogFile = NULL; // handle special string to set logs go to stdout
      alfDebugLog.setLogFile(debugLogFile, maxBytes, maxFiles, 1);
      alfDebugLog.setOutputFormat(SimpleLog::FormatOption::ShowTimeStamp | SimpleLog::FormatOption::ShowSeveritySymbol | SimpleLog::FormatOption::ShowMessage );
      alfDebugLog.info("ALF starting");
    } else {
      alfDebugLog.setLogFile("/dev/null");
    }

    if (mOptions.dimDnsNode != "") {
      Logger::get() << "Setting DIM_DNS_NODE from argument." << LogDebugDevel_(5001) << endm;
      Logger::get() << "DIM_DNS_NODE=" << mOptions.dimDnsNode << LogDebugDevel_(5001) << endm;
    } else if (const char* dimDnsNode = std::getenv("DIM_DNS_NODE")) {
      Logger::get() << "Picked up DIM_DMS_NODE from the environment." << LogDebugDevel_(5002) << endm;
      Logger::get() << "DIM_DNS_NODE=" << dimDnsNode << LogDebugDevel_(5002) << endm;
      mOptions.dimDnsNode = dimDnsNode;
    } else {
      BOOST_THROW_EXCEPTION(AlfException() << ErrorInfo::Message("DIM_DNS_NODE env variable not set, and no relevant argument provided.")); // InfoLogger and errors?
    }

    // Parse the default SWT word size
    SwtWord::Size swtWordSize = SwtWord::Size::Low;
    try {
      swtWordSize = SwtWord::sizeFromString(mOptions.swtWordSize);
    } catch (const ParseException& e) {
      Logger::get() << e.what() << LogWarningOps_(5003) << endm;
      Logger::get() << "SWT word size defaulting to low" << LogWarningOps_(5003) << endm;
    }

    std::string alfId = ip::host_name();
    boost::to_upper(alfId);

    Logger::get() << "Starting the DIM Server" << LogInfoDevel_(5004) << endm;
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
          Logger::get() << e.what() << LogWarningOps_(5005) << endm;
          continue;
        }
      }

      if (card.cardType == roc::CardType::Cru) {

        Logger::get() << "CRU " << card.serialId << " registered" << LogInfoDevel_(5006) << endm;
        bar = roc::ChannelFactory().getBar(card.serialId, 2);
        for (int linkId = 0; linkId < kCruNumLinks; linkId++) {
          links.push_back({ alfId, card.serialId, linkId, card.serialId.getEndpoint() * 12 + linkId, bar, roc::CardType::Cru });
        }

      } else if (card.cardType == roc::CardType::Crorc) {
        Logger::get() << "CRORC " << card.serialId << " registered" << LogInfoDevel_(5007) << endm;
        for (int linkId = 0; linkId < kCrorcNumLinks; linkId++) {
          bar = roc::ChannelFactory().getBar(card.serialId, linkId);
          links.push_back({ alfId, card.serialId, linkId, -1, bar, roc::CardType::Crorc });
        }
      } else {
        Logger::get() << card.serialId << " is not a CRU or a CRORC. Skipping..." << LogWarningDevel_(5008) << endm;
      }

      if (isVerbose()) {
        for (auto const& link : links) {
          Logger::get() << link.alfId << " " << link.serialId << " " << link.linkId << LogDebugDevel_(5009) << endm;
        }
      }
      alfServer.makeRpcServers(links, mOptions.sequentialRpcs);
    }

    alfDebugLog.info("Ready on DIM DNS %s with ALF id %s", mOptions.dimDnsNode.c_str(), alfId.c_str());

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
    std::string dimLogFileConfig = "";
  } mOptions;
};

} // namespace alf
} // namespace o2

int main(int argc, char** argv)
{
  return o2::alf::Alf().execute(argc, argv);
}
