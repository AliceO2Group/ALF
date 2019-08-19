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

#include <chrono>
#include <cstdlib>
#include <thread>

#include "AlfClient.h"
#include "Common/Program.h"
#include "Common/GuardFunction.h"
#include "DimServices/ServiceNames.h"
#include "Logger.h"

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
    options.add_options()("alf-id",
                          po::value<int>(&mOptions.alfId)->default_value(-1),
                          "The ID of the ALF server.");
    options.add_options()("dim-dns-node",
                          po::value<std::string>(&mOptions.dimDnsNode)->default_value(""),
                          "The DIM DNS node to connect to if the env var is not set");
    options.add_options()("serial",
                          po::value<int>(&mOptions.serial),
                          "CRU serial number");
    options.add_options()("link",
                          po::value<int>(&mOptions.link),
                          "Link number");
  }

  virtual void run(const po::variables_map&) override
  {

    getLogger() << "ALF client initializations..." << endm;

    if (const char* alfIdString = std::getenv("ALF_ID")) {
      getLogger() << "Picked up ALF_ID from the environment." << endm;
      getLogger() << "ALF_ID=" << alfIdString << endm;
      mOptions.alfId = atoi(alfIdString);
    } else {
      getLogger() << "ALF_ID env variable not set. Setting it from argument." << endm;
      //setenv("ALF_ID", std::to_string(mOptions.alfId).c_str(), 1);
      getLogger() << "ALF_ID=" << mOptions.alfId << endm;
      /* Do I need to set the env var for ALF_ID? */
    }

    if (const char* dimDnsNode = std::getenv("DIM_DNS_NODE")) {
      getLogger() << "Picked up DIM_DMS_NODE from the environment." << endm;
      getLogger() << "DIM_DNS_NODE=" << dimDnsNode << endm;
      mOptions.dimDnsNode = dimDnsNode;
    } else if (mOptions.dimDnsNode != "") {
      getLogger() << "DIM_DNS_NODE env variable not set. Setting it from argument." << endm;
      setenv("DIM_DNS_NODE", mOptions.dimDnsNode.c_str(), 1); // Don't be afraid to overwrite since we ended up here
      getLogger() << "DIM_DNS_NODE=" << mOptions.dimDnsNode << endm;
    } else {
      BOOST_THROW_EXCEPTION(AlfException() << ErrorInfo::Message("DIM_DNS_NODE env variable not set, and no relevant argument provided.")); // InfoLogger and errors?
    }

    getLogger() << "Starting the DIM Client using ALF ID=" << mOptions.alfId << ", serial=" << mOptions.serial << " and link=" << mOptions.link << endm;

    AlfLink link = AlfLink{ mOptions.alfId, mOptions.serial, mOptions.link, nullptr };

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

    auto swtOut = swtSequence.write({ std::make_pair("0x00000000000000000000", "write"),
                                      std::make_pair("0x000000001234", "write"),
                                      std::make_pair("0x0", "read") });
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
    int alfId = -1;
  } mOptions;
};

} // namespace Alf
} // namespace AliceO2

int main(int argc, char** argv)
{
  return AliceO2::Alf::ProgramAlfClient().execute(argc, argv);
}
