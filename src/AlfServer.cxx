#include <chrono>
#include <iomanip>
#include <thread>

#include "AlfServer.h"
#include "DimServices/ServiceNames.h"
#include "Swt/Swt.h"
#include "Visitor.h"
#include "Util.h"

namespace AliceO2
{
namespace Alf
{

AlfServer::AlfServer() : mCommandQueue(std::make_shared<CommandQueue>()), mRpcServers(), mServices()
{
}

std::string AlfServer::registerRead(const std::string& parameter, std::shared_ptr<roc::BarInterface> bar2)
{
  uint32_t address = Util::stringToHex(parameter);
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
 
  getLogger() << "registerWrite RPC, addr=" << Util::formatValue(address) << ", value=" << Util::formatValue(value) << endm;
  bar2->writeRegister(address / 4, value);
  return "";
}

std::string AlfServer::scaBlobWrite(const std::string& parameter, std::shared_ptr<roc::BarInterface> bar2, AlfLink link)
{
  std::vector<std::string> stringPairs = Util::split(parameter, argumentSeparator());
  std::vector<Sca::CommandData> commands = parseStringScaCommands(stringPairs);
  Sca sca = Sca(*bar2, link);
  return sca.writeSequence(commands);
}

std::string AlfServer::swtBlobWrite(const std::string& parameter, std::shared_ptr<roc::BarInterface> bar2, AlfLink link)
{
  
  std::vector<std::string> stringPairs = Util::split(parameter, argumentSeparator());
  std::vector<SwtWord> swtWords = parseStringSwtWords(stringPairs);
  Swt swt = Swt(*bar2, link);
  return swt.writeSequence(swtWords);
}

std::string AlfServer::publishRegistersStart(const std::string parameter, 
                                         std::shared_ptr<CommandQueue> commandQueue,
                                         AlfLink link)
{
  auto params = Util::split(parameter, argumentSeparator());

  if (params.size() < 3) {
    BOOST_THROW_EXCEPTION(AlfException() << ErrorInfo::Message("Not enough parameters given"));
  }

  std::string dnsName = params[0];
  std::string interval = params[1];

  // Convert register string sequence to binary format...
  std::vector<uintptr_t> registers;
  for (size_t i = 2; i < params.size(); i++) { //jump the dns name, and interval params
    registers.push_back(boost::lexical_cast<uintptr_t>(params[i]));
  }

  auto command = std::make_unique<CommandQueue::Command>();
  command->start = true;
  command->description.type = ServiceDescription::Register{std::move(registers)};
  command->description.dnsName = ServiceNames(link).publishRegisters(dnsName); //TODO: Can this be a bit cleaner?
  getLogger() << "dns name=" << dnsName << "and command desc=" << command->description.dnsName << endm;
  command->description.interval = std::chrono::milliseconds(int64_t(boost::lexical_cast<double>(interval) * 1000.0)); //TODO: ?
  command->description.link = link;

  getLogger() << "before tryaddtoqueue" << endm;
  //TODO: TO BE DEFINED IN REQS
  //std::string retDnsName(command->description.dnsName); //Copy the data before moving the pointer...
  tryAddToQueue(*commandQueue, std::move(command));
  //return retDnsName; // Return the dns name so that the publish caller can know which service to sub to
  return "";
}

std::string AlfServer::publishRegistersStop(const std::string parameter, 
                                        std::shared_ptr<CommandQueue> commandQueue,
                                        AlfLink link)
{
  auto command = std::make_unique<CommandQueue::Command>();
  command->start = false;
  command->description.type = ServiceDescription::Register();
  command->description.dnsName = ServiceNames(link).publishRegisters(parameter); //TODO: Can this be a bit cleaner?
  command->description.interval = std::chrono::milliseconds(0); //TODO: ?
  command->description.link = link;

  tryAddToQueue(*commandQueue, std::move(command));
  return ""; //TODO: ??
}

std::string AlfServer::publishScaSequenceStart(const std::string parameter, 
                                           std::shared_ptr<CommandQueue> commandQueue,
                                           AlfLink link)
{
  auto params = Util::split(parameter, argumentSeparator());

  if (params.size() < 3) {
    BOOST_THROW_EXCEPTION(AlfException() << ErrorInfo::Message("Not enough parameters given"));
  }

  std::string dnsName = params[0];
  std::string interval = params[1];

  std::vector<std::string> stringPairs = Util::split(parameter, argumentSeparator());
  ServiceDescription::ScaSequence scaSequence;
  scaSequence.commandDataPairs = parseStringScaCommands(stringPairs);

  auto command = std::make_unique<CommandQueue::Command>();
  command->start = true;
  command->description.type = ServiceDescription::Register();
  command->description.dnsName = ServiceNames(link).publishScaSequence(dnsName); //TODO: Can this be a bit cleaner?
  command->description.interval = std::chrono::milliseconds(int64_t(boost::lexical_cast<double>(interval) * 1000.0)); //TODO: ?
  command->description.link = link;

  tryAddToQueue(*commandQueue, std::move(command));
  return ""; //TODO: ??
}

std::string AlfServer::publishScaSequenceStop(const std::string parameter, 
                                          std::shared_ptr<CommandQueue> commandQueue,
                                          AlfLink link)
{
  auto command = std::make_unique<CommandQueue::Command>();
  command->start = false;
  command->description.type = ServiceDescription::Register();
  command->description.dnsName = ServiceNames(link).publishScaSequence(parameter); //TODO: Can this be a bit cleaner?
  command->description.interval = std::chrono::milliseconds(0); //TODO: ?
  command->description.link = link;

  tryAddToQueue(*commandQueue, std::move(command));
  return ""; //TODO: ??
}

std::string AlfServer::publishSwtSequenceStart(const std::string parameter, 
                                           std::shared_ptr<CommandQueue> commandQueue,
                                           AlfLink link)
{
  auto params = Util::split(parameter, argumentSeparator());

  if (params.size() < 3) {
    BOOST_THROW_EXCEPTION(AlfException() << ErrorInfo::Message("Not enough parameters given"));
  }

  std::string dnsName = params[0];
  std::string interval = params[1];

  std::vector<std::string> stringPairs = Util::split(parameter, argumentSeparator());
  ServiceDescription::SwtSequence swtSequence;
  swtSequence.swtWords = parseStringSwtWords(stringPairs);

  auto command = std::make_unique<CommandQueue::Command>();
  command->start = true;
  command->description.type = std::move(swtSequence);
  command->description.dnsName = ServiceNames(link).publishSwtSequence(dnsName); //TODO: Can this be a bit cleaner?
  command->description.interval = std::chrono::milliseconds(int64_t(boost::lexical_cast<double>(interval) * 1000.0)); //TODO: ?
  command->description.link = link;

  tryAddToQueue(*commandQueue, std::move(command));
  return ""; //TODO: ??

}

std::string AlfServer::publishSwtSequenceStop(const std::string parameter, 
                                          std::shared_ptr<CommandQueue> commandQueue,
                                          AlfLink link)
{
  auto command = std::make_unique<CommandQueue::Command>();
  command->start = false;
  command->description.type = ServiceDescription::Register();
  command->description.dnsName = ServiceNames(link).publishSwtSequence(parameter); //TODO: Can this be a bit cleaner?
  command->description.interval = std::chrono::milliseconds(0); //TODO: ?
  command->description.link = link;

  tryAddToQueue(*commandQueue, std::move(command));
  return ""; //TODO: ??
}

/*std::string AlfServer::scaPairSeparator()
{
  return ",";
}*/

Sca::CommandData AlfServer::stringToScaPair(std::string stringPair) {
  std::vector<std::string> scaPair = Util::split(stringPair, Sca::pairSeparator());
  if (stringPair.size() != 2) {
    BOOST_THROW_EXCEPTION(
        AlfException() << ErrorInfo::Message("SCA command-data pair not formatted correctly"));
  }
  Sca::CommandData commandData;
  commandData.command = Util::stringToHex(scaPair[0]); 
  commandData.data = Util::stringToHex(scaPair[1]); 
  return commandData;
}

/// Converts a 96-bit hex number string
SwtWord AlfServer::stringToSwtWord(const std::string& hexString)
{
  if (hexString.length() > 24) {
    BOOST_THROW_EXCEPTION(std::out_of_range("Parameter does not fit in 96-bit unsigned int"));
  }

  std::stringstream ss;
  ss << std::setw(24) << std::setfill('0') << hexString;

  SwtWord ret;
  ret.setLow(std::stoul(ss.str().substr(0, 8), NULL, 16));
  ret.setMed(std::stoul(ss.str().substr(8, 8), NULL, 16));
  ret.setHigh(std::stoul(ss.str().substr(16, 8), NULL, 16));

  return ret;
}

std::vector<Sca::CommandData> AlfServer::parseStringScaCommands(std::vector<std::string> stringPairs)
{
  std::vector<Sca::CommandData> pairs;
  for(const auto& stringPair : stringPairs) {
    if (stringPair.find('#') == 0) { // Isn't a comment
      pairs.push_back(stringToScaPair(stringPair));
    }
  }
  return pairs;
}

std::vector<SwtWord> AlfServer::parseStringSwtWords(std::vector<std::string> stringPairs)
{
  std::vector<SwtWord> pairs;
  for(const auto& stringPair : stringPairs) {
    if (stringPair.find('#') == 0) {
      pairs.push_back(stringToSwtWord(stringPair));
    }
  }
  return pairs;
}


bool AlfServer::tryAddToQueue(CommandQueue& commandQueue, std::unique_ptr<CommandQueue::Command> command)
{
  if (!commandQueue.write(std::move(command))) {
    getLogger() << InfoLogger::InfoLogger::Error << " command queue was full!" << endm;
    return false;
  }
  return true;
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
    auto &servers = mRpcServers[link.serial][link.linkId];
    std::shared_ptr<roc::BarInterface> bar2 = link.bar2; 
    auto commandQueue = mCommandQueue; // copy for lambda
 
    // Register Read
    servers.push_back(makeServer(names.registerRead(),
          [bar2](auto parameter){
          return registerRead(parameter, bar2);}));
    // Register Write
    servers.push_back(makeServer(names.registerWrite(),
          [bar2](auto parameter){
          return registerWrite(parameter, bar2);}));

/*    // SCA Sequence
    servers.push_back(makeServer(names.scaSequence(),
          [bar2, link](auto parameter){
          return scaBlobWrite(parameter, bar2, link);}));
    // SWT Sequence
    servers.push_back(makeServer(names.swtSequence(),
          [bar2, link](auto parameter){
          return swtBlobWrite(parameter, bar2, link);}));
*/
    // Publish Registers
    servers.push_back(makeServer(names.publishRegistersStart(),
          [commandQueue, link](auto parameter){
          return publishRegistersStart(parameter, commandQueue, link);}));
    servers.push_back(makeServer(names.publishRegistersStop(),
          [commandQueue, link](auto parameter){
          return publishRegistersStop(parameter, commandQueue, link);}));
/*
    // Publish SCA sequence
    servers.push_back(makeServer(names.publishScaSequenceStart(),
          [commandQueue, link](auto parameter){
          return publishScaSequenceStart(parameter, commandQueue, link);}));
    servers.push_back(makeServer(names.publishScaSequenceStop(),
          [commandQueue, link](auto parameter){
          return publishScaSequenceStop(parameter, commandQueue, link);}));

    // Publish SWT sequence
     servers.push_back(makeServer(names.publishSwtSequenceStart(),
          [commandQueue, link](auto parameter){
          return publishSwtSequenceStart(parameter, commandQueue, link);}));
    servers.push_back(makeServer(names.publishSwtSequenceStop(),
          [commandQueue, link](auto parameter){
          return publishSwtSequenceStop(parameter, commandQueue, link);}));
*/
  }
}

void AlfServer::addRemoveUpdateServices()
{
  addRemoveServices();
  updateServices();
}

void AlfServer::addRemoveServices()
{
  std::unique_ptr<CommandQueue::Command> command;
  while(mCommandQueue->read(command)) {
    if (command->start) {
      addService(command->description);
    } else {
      removeService(command->description.dnsName);
    }
  }
}

void AlfServer::addService(const ServiceDescription& serviceDescription)
{
  getLogger() << "In add service for : " << serviceDescription.dnsName << endm;
  if (mServices.count(serviceDescription.dnsName)) {
    removeService(serviceDescription.dnsName);
  }

  auto service = std::make_unique<Service>();
  service->description = serviceDescription;
  service->nextUpdate = std::chrono::steady_clock::now();

  Visitor::apply(service->description.type,
      [&](const ServiceDescription::Register& type) {
      // Estimate max needed size. I'm not sure DIM can handle reallocations of this buffer, so we avoid that...
      service->buffer.resize(type.addresses.size()*20 + 512);
      //if (verbose) getLogger() << "Starting SCA publisher '" << service->description.dnsName << "' with "
      //  << type.addresses.size() << " address(es) at interval "
      //  << service->description.interval.count() << "ms" << endm;
      },
      [&](const ServiceDescription::ScaSequence& type) {
      // Estimate max needed size. I'm not sure DIM can handle reallocations of this buffer, so we avoid that...
      service->buffer.resize(type.commandDataPairs.size()*20 + 512);
      //if (verbose) getLogger() << "Starting SCA publisher '" << service->description.dnsName << "' with "
      //  << type.commandDataPairs.size() << " command(s) at interval "
      //  << service->description.interval.count() << "ms" << endm;
      },
      [&](const ServiceDescription::SwtSequence& type) {
      // Estimate max needed size. I'm not sure DIM can handle reallocations of this buffer, so we avoid that...
      service->buffer.resize(type.swtWords.size()*20 + 512);
      //if (verbose) getLogger() << "Starting SCA publisher '" << service->description.dnsName << "' with "
      //  << type.swtWords.size() << " SWT word(s) at interval "
      //  << service->description.interval.count() << "ms" << endm;
      }
  );
  
  std::fill(service->buffer.begin(), service->buffer.end(), '\0');
  service->dimService = std::make_unique<DimService>(service->description.dnsName.c_str(),
      "C", //"C" means 8bit character
      service->buffer.data(),
      Util::strlenMax(service->buffer.data(), service->buffer.size()));
  mServices.insert(std::make_pair(serviceDescription.dnsName, std::move(service)));
}

void AlfServer::removeService(const std::string& dnsName)
{
  getLogger() << "In remove service" << endm;
  mServices.erase(dnsName);
}

void AlfServer::updateServices()
{
  getLogger() << "In update services" << endm;
  auto now = std::chrono::steady_clock::now();
  std::chrono::steady_clock::time_point next = now + std::chrono::seconds(1);

  for (auto& kv : mServices) {
    auto& service = *kv.second;
    if (service.nextUpdate < now) {
      updateService(service);
      service.advanceUpdateTime();
      next = std::min(next, service.nextUpdate);
    }
  }

  std::this_thread::sleep_until(next);
}

void AlfServer::updateService(Service& service)
{
  std::string result;

  Visitor::apply(service.description.type,
      [&](const ServiceDescription::Register& type) {
        std::stringstream ss;
        auto bar2 = service.description.link.bar2.get();
        for (size_t i = 0; i < type.addresses.size(); i++) {
        auto value = bar2->readRegister(type.addresses[i] / 4);
        ss << Util::formatValue(value) << "\n";
        }
        result = ss.str();
      },
      [&](const ServiceDescription::ScaSequence& type) {
        auto bar2 = service.description.link.bar2.get();
        auto sca = Sca(*bar2, service.description.link);
        result = sca.writeSequence(type.commandDataPairs);
      },
      [&](const ServiceDescription::SwtSequence& type) {
        auto bar2 = service.description.link.bar2.get();
        auto swt = Swt(*bar2, service.description.link);
        result = swt.writeSequence(type.swtWords);
      }
  );

  // Reset and copy into the persistent buffer because I don't trust DIM with the non-persistent std::string
  std::fill(service.buffer.begin(), service.buffer.end(), '\0');
  std::copy(result.begin(), result.end(), service.buffer.begin());
  service.dimService->updateService(service.buffer.data(), result.size() + 1);
}

} // namespace Alf
} // namespace AliceO2
