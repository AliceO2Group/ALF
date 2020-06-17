// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file AlfClient.cxx
/// \brief Definition of the command line tool to run an ALF client
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/asio.hpp>
#include <chrono>
#include <cstdlib>
#include <thread>

#include "AlfClient.h"
#include "Common/Program.h"
#include "Common/GuardFunction.h"
#include "DimServices/ServiceNames.h"
#include "Logger.h"

namespace ip = boost::asio::ip;
namespace po = boost::program_options;

namespace AliceO2
{
namespace Alf
{

AliceO2::InfoLogger::InfoLogger logger;

class AlfClient : public AliceO2::Common::Program
{
 public:
  AlfClient()
  {
  }

  virtual Description getDescription() override
  {
    return { "ALF DIM Client", "ALICE Low-level Front-end DIM client", "o2-alf-client" };
  }

  virtual void addOptions(po::options_description& options) override
  {
    options.add_options()("dim-dns-node",
                          po::value<std::string>(&mOptions.dimDnsNode)->default_value(""),
                          "The DIM DNS node to connect to if the env var is not set");
    options.add_options()("card-sequence",
                          po::value<int>(&mOptions.cardSequence),
                          "Card sequence number");
    options.add_options()("link",
                          po::value<int>(&mOptions.link),
                          "Link number");
    options.add_options()("alf-id",
                          po::value<std::string>(&mOptions.alfId)->default_value(""),
                          "Hostname of node running the ALF server(required)");
    options.add_options()("crorc",
                          po::bool_switch(&mOptions.crorc)->default_value(false),
                          "Flag enabling the test of the crorc (exclusive)");
    options.add_options()("ic",
                          po::bool_switch(&mOptions.ic)->default_value(false),
                          "Flag enabling the ic tests");
    options.add_options()("lla",
                          po::bool_switch(&mOptions.lla)->default_value(false),
                          "Flag enabling the lla tests");
    options.add_options()("sca",
                          po::bool_switch(&mOptions.sca)->default_value(false),
                          "Flag enabling the sca tests");
    options.add_options()("swt",
                          po::bool_switch(&mOptions.swt)->default_value(false),
                          "Flag enabling the swt tests");
    options.add_options()("pattern-player",
                          po::bool_switch(&mOptions.patternPlayer)->default_value(false),
                          "Flag enabling the pattern player tests");
    options.add_options()("swt-stress",
                          po::bool_switch(&mOptions.swtStress)->default_value(false),
                          "Flag enabling the swt-stress tests");
    options.add_options()("swt-stress-cycles",
                          po::value<int>(&mOptions.swtStressCycles)->default_value(2),
                          "Number of cycles for which to run the SWT stress test");
    options.add_options()("swt-stress-words",
                          po::value<int>(&mOptions.swtStressWords)->default_value(1000),
                          "Number of SWT words to write and read in one go");
  }

  virtual void run(const po::variables_map&) override
  {
    if (mOptions.alfId == "") {
      getErrorLogger() << "Parameter alf-id is required." << endm;
      return;
    }

    getLogger() << "ALF client initializations..." << endm;

    if (mOptions.dimDnsNode != "") {
      getLogger() << "Setting DIM_DNS_NODE from argument." << endm;
      getLogger() << "DIM_DNS_NODE=" << mOptions.dimDnsNode << endm;
      setenv("DIM_DNS_NODE", mOptions.dimDnsNode.c_str(), true);
    } else if (const char* dimDnsNode = std::getenv("DIM_DNS_NODE")) {
      getLogger() << "Picked up DIM_DMS_NODE from the environment." << endm;
      getLogger() << "DIM_DNS_NODE=" << dimDnsNode << endm;
      mOptions.dimDnsNode = dimDnsNode;
    } else {
      BOOST_THROW_EXCEPTION(AlfException() << ErrorInfo::Message("DIM_DNS_NODE env variable not set, and no relevant argument provided.")); // InfoLogger and errors?
    }

    std::string alfId = mOptions.alfId;
    boost::to_upper(alfId);

    getLogger() << "Starting the DIM Client using ALF ID=" << alfId << ", card #=" << mOptions.cardSequence << " and link=" << mOptions.link << endm;

    AlfLink link = AlfLink{ alfId, mOptions.cardSequence, mOptions.link, nullptr, roc::CardType::Cru };

    ServiceNames names(link);

    if (mOptions.crorc) {
      link.cardType = roc::CardType::Crorc;
      Alf::RegisterSequenceRpc registerSequence(names.registerSequence());
      auto regOut = registerSequence.write({ std::make_pair("0x19c", ""),
                                             std::make_pair("0xa0", ""),
                                             std::make_pair("0x1f0", ""),
                                             std::make_pair("0x1f0", "0x00080000"),
                                             std::make_pair("0x1f0", "") });
      getLogger() << "[REGISTER SEQUENCE] output: " << regOut << endm;

      return;
    }

    // Only CRU from this point forward
    Alf::RegisterReadRpc registerReadRpc(names.registerRead());
    Alf::RegisterWriteRpc registerWriteRpc(names.registerWrite());
    Alf::PatternPlayerRpc patternPlayerRpc(names.patternPlayer());
    Alf::LlaSessionStartRpc llaSessionStartRpc(names.llaSessionStart());
    Alf::LlaSessionStopRpc llaSessionStopRpc(names.llaSessionStop());

    Alf::SwtSequenceRpc swtSequence(names.swtSequence());
    Alf::ScaSequenceRpc scaSequence(names.scaSequence());
    Alf::IcSequenceRpc icSequence(names.icSequence());
    Alf::IcGbtI2cWriteRpc icGbtI2cWriteRpc(names.icGbtI2cWrite());

    // Test register write and read
    uint32_t wAddress = 0x00f00078;
    uint32_t wValue = 0x4;
    uint32_t rAddress = 0x00f0005c;
    registerWriteRpc.writeRegister(wAddress, wValue);
    uint32_t rValue = registerReadRpc.readRegister(rAddress);
    getLogger() << "[REGISTER] Wrote: " << Util::formatValue(wValue) << " Read: " << Util::formatValue(rValue) << endm;

    if (mOptions.swt) {
      auto swtOut = swtSequence.write({ std::make_pair("0x0000000000000000000", "write"),
                                        std::make_pair("", "reset"),
                                        std::make_pair("0x000000001234", "write"),
                                        std::make_pair("", "read"),
                                        std::make_pair("0xdeadbeef", "write"),
                                        std::make_pair("1", "read"),
                                        std::make_pair("0xbadc0ffee", "write"),
                                        std::make_pair("4", "read") });
      getLogger() << "[SWT_SEQUENCE] output: " << swtOut << endm;
    }

    if (mOptions.swtStress) {
      for (int cycle = 0; cycle < mOptions.swtStressCycles; cycle++) {
        // Make the input pairs
        std::vector<std::pair<std::string, std::string>> swtStressPairs;
        swtStressPairs.push_back(std::make_pair("", "reset"));
        for (int i = 0; i < mOptions.swtStressWords; i++) {
          std::stringstream ss;
          ss << "0x" << std::hex << i;
          swtStressPairs.push_back(std::make_pair(ss.str(), "write"));
        }
        swtStressPairs.push_back(std::make_pair("1000", "read"));

        auto swtStressOut = swtSequence.write(swtStressPairs);
        getLogger() << "[SWT stress] cycle  " << cycle << endm;
        //getLogger() << "[SWT stress] output:  " << swtStressOut << endm;
        std::cout << "[SWT stress] output:  " << swtStressOut << std::endl; // Infologger gets filled up here...
      }
    }

    if (mOptions.sca) {
      auto scaOut = scaSequence.write({ std::make_pair("1000", "wait"),
                                        std::make_pair("0x00010002", "0xff000000"),
                                        std::make_pair("0x00020004", "0xff000000"),
                                        std::make_pair("0x00030006", "0xff000000"),
                                        std::make_pair("0x0B950282", "0x50010000"),
                                        std::make_pair("0x0B9601DE", "0x50000000"),
                                        std::make_pair("0x0B970471", "0x50000000"),
                                        std::make_pair("0x0B980461", "0x50000000") });
      getLogger() << "[SCA_SEQUENCE] output: " << scaOut << endm;
    }

    if (mOptions.ic) {
      auto icOut = icSequence.write({
        std::make_pair("0x54,0xff", "write"),
        std::make_pair("0x54", "read"),
        std::make_pair("0x55,0xff", "write"),
        std::make_pair("0x55", "read"),
        std::make_pair("0x56,0xff", "write"),
        std::make_pair("0x56", "read"),
      });
      getLogger() << "[IC_SEQUENCE] output: " << icOut << endm;

      icGbtI2cWriteRpc.write(0x3);
    }

    if (mOptions.patternPlayer) {
      getLogger() << "Running the pattern player" << endm;
      auto ppOut = patternPlayerRpc.play({
        "0x23456789abcdef123456",
        "0x5678",
        "0x9abc",
        "42",
        "0",
        "53",
        "30", // comment to test case of less parameters than expected
        "29",
        "#a comment", // tests that a comment is parsed gracfully
        "false",
        "true",
        "false",
        //"0xdeadbeef" // Uncomment to test more parameters than expected
      });
      getLogger() << "Pairs test return: " << ppOut << endm;
    }

    if (mOptions.lla) {
      getLogger() << "Running the lla" << endm;
      const auto start = std::chrono::steady_clock::now();
      auto timeExceeded = [&]() { return ((std::chrono::steady_clock::now() - start) > std::chrono::milliseconds(4100)); };

      while (!timeExceeded()) {
        //auto llaOut = llaSessionStartRpc.write("alf_client_test", 0);
        auto llaOut = llaSessionStartRpc.write("alf_client_test");
        if (llaOut == "success\n") {
          std::this_thread::sleep_for(std::chrono::seconds(4));
          llaOut = llaSessionStopRpc.write("");
          break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // This sleep is necessary to unblock the DIM RPC channel so the other thread can unlock...
                                                                    // However, we cannot know how quickly the other thread will be successful in running the RPC call
      }
    }

    getLogger() << "See ya!" << endm;
  }

 private:
  struct OptionsStruct {
    std::string dimDnsNode = "";
    int cardSequence = -1;
    int link = -1;
    std::string alfId = "";
    bool crorc = false;
    bool ic = false;
    bool lla = false;
    bool sca = false;
    bool swt = false;
    bool patternPlayer = false;
    bool swtStress = false;
    int swtStressCycles = 2;
    int swtStressWords = 1000;
  } mOptions;
};

} // namespace Alf
} // namespace AliceO2

int main(int argc, char** argv)
{
  return AliceO2::Alf::AlfClient().execute(argc, argv);
}
