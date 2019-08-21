// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file ProgramAlfClient.cxx
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

class ProgramAlfClient : public AliceO2::Common::Program
{
 public:
  ProgramAlfClient()
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
    options.add_options()("serial",
                          po::value<int>(&mOptions.serial),
                          "CRU serial number");
    options.add_options()("link",
                          po::value<int>(&mOptions.link),
                          "Link number");
    options.add_options()("alf-id",
                          po::value<std::string>(&mOptions.alfId)->default_value(""),
                          "Hostname of node running the ALF server(required)");
  }

  virtual void run(const po::variables_map&) override
  {
    if (mOptions.alfId == "") {
      getErrorLogger() << "Parameter alf-id is required." << endm;
      return;
    }

    getLogger() << "ALF client initializations..." << endm;

    if (mOptions.dimDnsNode != "") {
      getLogger() << "DIM_DNS_NODE env variable not set. Setting it from argument." << endm;
      getLogger() << "DIM_DNS_NODE=" << mOptions.dimDnsNode << endm;
    } else if (const char* dimDnsNode = std::getenv("DIM_DNS_NODE")) {
      getLogger() << "Picked up DIM_DMS_NODE from the environment." << endm;
      getLogger() << "DIM_DNS_NODE=" << dimDnsNode << endm;
      mOptions.dimDnsNode = dimDnsNode;
    } else {
      BOOST_THROW_EXCEPTION(AlfException() << ErrorInfo::Message("DIM_DNS_NODE env variable not set, and no relevant argument provided.")); // InfoLogger and errors?
    }

    std::string alfId = mOptions.alfId; //TODO: change to hostname argument
    boost::to_upper(alfId);

    getLogger() << "Starting the DIM Client using ALF ID=" << alfId << ", serial=" << mOptions.serial << " and link=" << mOptions.link << endm;

    AlfLink link = AlfLink{ alfId, mOptions.serial, mOptions.link, nullptr };

    ServiceNames names(link);
    Alf::RegisterReadRpc registerReadRpc(names.registerRead());
    Alf::RegisterWriteRpc registerWriteRpc(names.registerWrite());
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
    getWarningLogger() << "Wrote: " << Util::formatValue(wValue) << " Read: " << Util::formatValue(rValue) << endm;

    auto swtOut = swtSequence.write({ std::make_pair("0x0000000000000000000", "write"),
                                      std::make_pair("", "reset"),
                                      std::make_pair("0x000000001234", "write"),
                                      std::make_pair("", "read") });
    getWarningLogger() << "swtSequence output: " << swtOut << endm;

    auto scaOut = scaSequence.write({ std::make_pair("0x00010002", "0xff000000"),
                                      std::make_pair("0x00020004", "0xff000000"),
                                      std::make_pair("0x00030006", "0xff000000"),
                                      std::make_pair("0x0B950282", "0x50010000"),
                                      std::make_pair("0x0B9601DE", "0x50000000"),
                                      std::make_pair("0x0B970471", "0x50000000"),
                                      std::make_pair("0x0B980461", "0x50000000") });
    getWarningLogger() << "scaSequence output: " << scaOut << endm;

    auto icOut = icSequence.write({
      std::make_pair("0x54,0xff", "write"),
      std::make_pair("0x54", "read"),
      std::make_pair("0x55,0xff", "write"),
      std::make_pair("0x55", "read"),
      std::make_pair("0x56,0xff", "write"),
      std::make_pair("0x56", "read"),
    });
    getWarningLogger() << "icSequence output: " << icOut << endm;

    icGbtI2cWriteRpc.write(0x3);

    getWarningLogger() << "See ya!" << endm;
  }

 private:
  struct OptionsStruct {
    std::string dimDnsNode = "";
    int serial = -1;
    int link = -1;
    std::string alfId = "";
  } mOptions;
};

} // namespace Alf
} // namespace AliceO2

int main(int argc, char** argv)
{
  return AliceO2::Alf::ProgramAlfClient().execute(argc, argv);
}
