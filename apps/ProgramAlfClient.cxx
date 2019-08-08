#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

#include "AlfClient.h"
#include "AlfServer.h" // For the logger for now
//#include "Common.h"
#include "Common/Program.h"
#include "DimServices/ServiceNames.h"

namespace po = boost::program_options;

namespace AliceO2 
{
namespace Alf
{

class ProgramAlfClient : public AliceO2::Common::Program
{
 public:
   ProgramAlfClient()
   {
   }

   virtual Description getDescription() override
   {
     return {"ALF DIM Client", "ALICE Low-level Front-end DIM client", "o2-alf-client"};
   }

   virtual void addOptions(po::options_description& options) override
   {
     options.add_options()
       ("dim-dns-node",
        po::value<std::string>(&mOptions.dimDnsNode)->default_value(""),
        "The DIM DNS node to connect to if the env var is not set")
       ("serial",
        po::value<int>(&mOptions.serial),
        "CRU serial number")
       ("link",
        po::value<int>(&mOptions.link),
        "Link number");
   }

   virtual void run(const po::variables_map&) override {

     getLogger() << "ALF client initializations..." << endm;
     
     if (const char* dimDnsNode = std::getenv("DIM_DNS_NODE")) {
       getLogger() << "DIM_DNS_NODE=" << dimDnsNode << endm;
       mOptions.dimDnsNode = dimDnsNode;
     } else if (mOptions.dimDnsNode != "") {
       getLogger() << "DIM_DNS_NODE env variable not set. Setting it from argument." << endm;
       setenv("DIM_DNS_NODE", mOptions.dimDnsNode.c_str(), 1); // Don't be afraid to overwrite since we ended up here
       getLogger() << "DIM_DNS_NODE=" << mOptions.dimDnsNode << endm;
     } else {
       BOOST_THROW_EXCEPTION(AlfException() << ErrorInfo::Message("DIM_DNS_NODE env variable not set, and no relevant argument provided.")); // InfoLogger and errors?
     }

     getLogger() << "Starting the DIM Client using serial=" << mOptions.serial << " and link=" << mOptions.link << endm;

     AlfLink link = AlfLink{-1, mOptions.serial, mOptions.link, nullptr};

     ServiceNames names(link);
     Alf::RegisterReadRpc registerReadRpc(names.registerRead());
     Alf::RegisterWriteRpc registerWriteRpc(names.registerWrite());
     Alf::PublishRegistersStartRpc publishRegistersStartRpc(names.publishRegistersStart());
     Alf::PublishRegistersStopRpc publishRegistersStopRpc(names.publishRegistersStop());

     // Test register write and read
     uint32_t wAddress = 0x00f00078;
     uint32_t wValue = 0x4;
     uint32_t rAddress = 0x00f0005c;
     registerWriteRpc.writeRegister(wAddress, wValue);
     uint32_t rValue = registerReadRpc.readRegister(rAddress);
     std::cout << "Wrote: " << Util::formatValue(wValue) << " Read: " << Util::formatValue(rValue) << std::endl;


     // Test register publishing
     std::cout << "Publishing" << std::endl;
     std::cout << publishRegistersStartRpc.publish("TEST_PUB_REGS_SINGLE", 1.0, {rAddress}) << std::endl;
     std::cout << publishRegistersStartRpc.publish("TEST_PUB_REGS_MULTI", 2.0, {rAddress, rAddress+4, rAddress+8}) << std::endl;
     
     Alf::PublishInfo publishRegistersInfoSingle(names.publishRegisters("TEST_PUB_REGS_SINGLE"));
     Alf::PublishInfo publishRegistersInfoMulti(names.publishRegisters("TEST_PUB_REGS_MULTI"));

     std::this_thread::sleep_for(std::chrono::seconds(6));

     publishRegistersStopRpc.stop("TEST_PUB_REGS_SINGLE");
     publishRegistersStopRpc.stop("TEST_PUB_REGS_MULTI");

   }

 private:
   struct OptionsStruct {
     std::string dimDnsNode = "";
     int serial = -1;
     int link = -1;
   } mOptions;

};

} // namespace AliceO2
} // namespace Alf

int main(int argc, char** argv)
{
  return AliceO2::Alf::ProgramAlfClient().execute(argc, argv);
}
