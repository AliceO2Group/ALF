#ifndef ALICEO2_ALF_ALF_H_
#define ALICEO2_ALF_ALF_H_

#include <chrono>
#include <iomanip>
#include <thread>

#include "AlfException.h"
#include "DimServices/DimServices.h"
#include "Common.h"

#include "folly/ProducerConsumerQueue.h"
#include "InfoLogger/InfoLogger.hxx"
#include "ReadoutCard/BarInterface.h"

#include <boost/algorithm/string/predicate.hpp>

constexpr auto endm = AliceO2::InfoLogger::InfoLogger::endm;

static AliceO2::InfoLogger::InfoLogger& getLogger()
{
  static AliceO2::InfoLogger::InfoLogger logger;
  return logger;
}

namespace roc = AliceO2::roc;

namespace AliceO2
{
namespace Alf
{

class CommandQueue
{
 public:
   struct Command
   {
     bool start; ///< true starts service, false stops!
     ServiceDescription description;
   };

   bool write(std::unique_ptr<Command> command)
   {
     return mQueue.write(std::move(command));
   }

   bool read(std::unique_ptr<Command>& command)
   {
     return mQueue.read(command);
   }

 private:
   folly::ProducerConsumerQueue<std::unique_ptr<Command>> mQueue {512};
};

/// Length of the success/failure prefix that's returned in RPC calls
constexpr size_t PREFIX_LENGTH(8);

//constexpr std::string_view ARGUMENT_SEPARATOR("\n");
//constexpr std::string_view SCA_PAIR_SEPARATOR(",");

//bool isSuccess(const std::string& string);
//bool isFail(const std::string& string);
//std::string stripPrefix(const std::string& string);

class AlfServer
{
  public:
    AlfServer();
    /*void makeServers(std::map<int, std::map<int, std::shared_ptr<roc::BarInterface>>> bars,
        std::vector<AlfLink> links,
        std::shared_ptr<CommandQueue> commandQueue,
        std::map<int, std::map<int, std::vector<std::unique_ptr<StringRpcServer>>>> &rpcServers);*/
    void makeRpcServers(std::shared_ptr<roc::BarInterface> bar2, std::vector<AlfLink> links);


    void addRemoveUpdateServices();

  private:

    void addRemoveServices();
    void addService(const ServiceDescription& serviceDescription);
    void removeService(const std::string& dnsName);
    void updateServices();
    void updateService(Service& service);

    static bool tryAddToQueue(CommandQueue& commandQueue, std::unique_ptr<CommandQueue::Command> command);

    static std::string registerRead(const std::string& parameter, std::shared_ptr<roc::BarInterface>);
    static std::string registerWrite(const std::string& parameter, std::shared_ptr<roc::BarInterface>);
    static std::string scaBlobWrite(const std::string& parameter, std::shared_ptr<roc::BarInterface>, AlfLink link);
    static std::string swtBlobWrite(const std::string& parameter, std::shared_ptr<roc::BarInterface>, AlfLink link);
    static std::string publishRegistersStart(const std::string parameter, 
                                             std::shared_ptr<CommandQueue> commandQueue,
                                             AlfLink link);
    static std::string publishRegistersStop(const std::string parameter, 
                                            std::shared_ptr<CommandQueue> commandQueue,
                                            AlfLink link);
    static std::string publishScaSequenceStart(const std::string parameter, 
                                               std::shared_ptr<CommandQueue> commandQueue,
                                               AlfLink link);
    static std::string publishScaSequenceStop(const std::string parameter, 
                                              std::shared_ptr<CommandQueue> commandQueue,
                                              AlfLink link);
    static std::string publishSwtSequenceStart(const std::string parameter, 
                                               std::shared_ptr<CommandQueue> commandQueue,
                                               AlfLink link);
    static std::string publishSwtSequenceStop(const std::string parameter, 
                                              std::shared_ptr<CommandQueue> commandQueue,
                                            AlfLink link);

    static std::string scaPairSeparator();
    static Sca::CommandData stringToScaPair(std::string stringPair);
    static SwtWord stringToSwtWord(const std::string& hexString);
    static std::vector<Sca::CommandData> parseStringScaCommands(std::vector<std::string> stringPairs);
    static std::vector<SwtWord> parseStringSwtWords(std::vector<std::string> stringPairs);

    /// serial -> BAR number -> BAR Interface
    //std::map<int, std::map<int, std::shared_ptr<roc::BarInterface>>> mBars; //BAR number is necessary here because of CRORC

    /// Command queue for passing commands from DIM RPC calls (which are in separate threads) to the main program loop
    std::shared_ptr<CommandQueue> mCommandQueue;

    /// serial -> link -> vector of RPC servers
    std::map<int, std::map<int, std::vector<std::unique_ptr<StringRpcServer>>>> mRpcServers;

    /// Object representing a publishing DIM service
    std::map<std::string, std::unique_ptr<Service>> mServices;
};

} // namespace Alf
} // namespace AliceO2

#endif // ALICEO2_ALF_ALF_H_
