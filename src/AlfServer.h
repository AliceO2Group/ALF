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
/// \brief Definition of ALF server related classes & functions
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef ALICEO2_ALF_ALFSERVER_H_
#define ALICEO2_ALF_ALFSERVER_H_

#include <chrono>
#include <iomanip>
#include <thread>

#include "AlfException.h"
#include "DimServices/DimServices.h"
#include "Common.h"

#include "folly/ProducerConsumerQueue.h"

#include <boost/algorithm/string/predicate.hpp>

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

class AlfServer
{
  public:
    AlfServer();
    void makeRpcServers(std::vector<AlfLink> links);


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

#endif // ALICEO2_ALF_ALFSERVER_H_
