
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

/// \file AlfScClient.cxx
/// \brief Definition of the command line tool to run an ALF client
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <chrono>
#include <thread>

#include "Common/Program.h"
#include "Alf/Alf.h"
#include "Util.h"

namespace po = boost::program_options;

namespace o2
{
namespace alf
{

class AlfLibClient : public AliceO2::Common::Program
{
 public:
  AlfLibClient()
  {
  }

  virtual Description getDescription() override
  {
    return { "ALF Library Client", "ALICE Low-level Front-end Library client", "o2-alf-lib-client" };
  }

  virtual void addOptions(po::options_description& options) override
  {
    options.add_options()("link",
                          po::value<int>(&mOptions.link)->default_value(0),
                          "Link number");
    options.add_options()("ic",
                          po::bool_switch(&mOptions.ic)->default_value(false),
                          "Flag enabling the ic tests");
    options.add_options()("sca",
                          po::bool_switch(&mOptions.sca)->default_value(false),
                          "Flag enabling the sca tests");
    options.add_options()("swt",
                          po::bool_switch(&mOptions.swt)->default_value(false),
                          "Flag enabling the swt tests");
    options.add_options()("serial",
                          po::value<int>(&mOptions.serial)->default_value(-1),
                          "Serial to use");
    options.add_options()("endpoint",
                          po::value<int>(&mOptions.endpoint)->default_value(0),
                          "Endpoint to use");
  }

  virtual void run(const po::variables_map&) override
  {
    kDebugLogging = isVerbose();

    Logger::enableInfoLogger(false);
    Logger::setFacility("ALF/LibClient");
    if (mOptions.sca) {
      std::cout << "Running SCA test" << std::endl;
      auto sca = Sca(roc::SerialId{ mOptions.serial, mOptions.endpoint }, mOptions.link);
      sca.scReset();

      std::cout << "Running simple SCA operations" << std::endl;
      try {
        sca.svlReset();
        sca.svlConnect();
        auto scaOut = sca.executeCommand({ 0x00010002, 0xff000000 });
        std::cout << scaOut.command << " " << scaOut.data << std::endl;
      } catch (const ScaException& e) {
        std::cerr << e.what() << std::endl;
      }

      std::cout << "Running an SCA sequence" << std::endl;
      std::vector<std::pair<Sca::Operation, Sca::Data>> ops;
      sca.setChannel(1);
      ops.push_back({ Sca::Operation::SCReset, {} });
      ops.push_back({ Sca::Operation::SVLReset, {} });
      ops.push_back({ Sca::Operation::SVLConnect, {} });
      ops.push_back({ Sca::Operation::Command, Sca::CommandData{ 0x00100002, 0xff000000 } });
      ops.push_back({ Sca::Operation::Command, Sca::CommandData{ 0x00100003, 0xff000000 } });
      ops.push_back({ Sca::Operation::Wait, 100 });
      ops.push_back({ Sca::Operation::Command, Sca::CommandData{ 0x00100004, 0xff000000 } });
      auto output = sca.executeSequence(ops);
      for (const auto& out : output) {
        if (out.first == Sca::Operation::Command) {
          std::cout << "Command: " << boost::get<Sca::CommandData>(out.second) << std::endl;
        } else if (out.first == Sca::Operation::Wait) {
          std::cout << "Wait: " << std::dec << boost::get<Sca::WaitTime>(out.second) << std::endl;
        } else if (out.first == Sca::Operation::SVLReset) {
          std::cout << "SVL Reset" << std::endl;
        } else if (out.first == Sca::Operation::SCReset) {
          std::cout << "SC Reset" << std::endl;
        } else if (out.first == Sca::Operation::SVLConnect) {
          std::cout << "SVL Connect " << std::endl;
        } else if (out.first == Sca::Operation::Error) {
          std::cout << "Error: " << boost::get<std::string>(out.second) << std::endl;
        } else {
          std::cout << "Unknown operation" << std::endl;
        }
      }
    }

    if (mOptions.swt) {
      std::cout << "Running SWT test" << std::endl;
      auto swt = Swt(roc::SerialId{ mOptions.serial, mOptions.endpoint }, mOptions.link);

      std::cout << "Running simple SWT operations" << std::endl;
      try {
        swt.scReset();
        swt.write({ 0xcafe, 0x41d, 0x0 });
        swt.write({ 0xb00f, 0x42, 0x88 });
        swt.write({ 0xb00f, 0x42, 0x88 });
        swt.write({ 0xbe0f, 0x0, 0x0, SwtWord::Size::High });
        swt.write({ 0xb00f, 0x42, 0x21, SwtWord::Size::Low });
        auto wordsRead = swt.read(SwtWord::Size::Medium, 10);
        for (const auto& word : wordsRead) {
          std::cout << word << std::endl;
        }
      } catch (const SwtException& e) {
        std::cerr << e.what() << std::endl;
      }

      std::cout << "Running an SWT sequence" << std::endl;
      std::vector<std::pair<Swt::Operation, Swt::Data>> ops;
      swt = Swt(roc::SerialId{ mOptions.serial, mOptions.endpoint });
      swt.setChannel(1);
      ops.push_back({ Swt::Operation::SCReset, {} });
      ops.push_back({ Swt::Operation::Write, SwtWord{ 0xcaff, 0x41d, 0x0 } });
      ops.push_back({ Swt::Operation::Write, SwtWord{ 0xb00f, 0x42, 0x88, SwtWord::Size::High } });
      ops.push_back({ Swt::Operation::Write, SwtWord{ 0xb00f, 0x42, 0x88 } });
      ops.push_back({ Swt::Operation::Read, {} });
      ops.push_back({ Swt::Operation::Wait, 100 });
      ops.push_back({ Swt::Operation::Write, SwtWord{ 0x1, 0x0, 0x0, SwtWord::Size::Low } });
      ops.push_back({ Swt::Operation::Write, SwtWord{ 0xb00f, 0x42, 0x88, SwtWord::Size::Low } });
      ops.push_back({ Swt::Operation::Write, SwtWord{ 0xcafe, 0x41d, 0x0 } });
      ops.push_back({ Swt::Operation::Read, {} });
      ops.push_back({ Swt::Operation::Write, SwtWord{ 0x42, 0x0, 0x0, SwtWord::Size::Low } });
      ops.push_back({ Swt::Operation::Write, SwtWord{ 0xbad, 0x88, 0x43, SwtWord::Size::Low } });
      ops.push_back({ Swt::Operation::Write, SwtWord{ 0xcafe, 0x41d, 0x0 } });
      ops.push_back({ Swt::Operation::Read, { 50 } });
      ops.push_back({ Swt::Operation::Error, {} });
      auto output = swt.executeSequence(ops, true); // execute the sequence atomically
      for (const auto& out : output) {
        if (out.first == Swt::Operation::Write) {
          std::cout << "Write | " << boost::get<SwtWord>(out.second) << std::endl;
        } else if (out.first == Swt::Operation::Read) {
          std::cout << "Read  | " << boost::get<SwtWord>(out.second) << std::endl;
        } else if (out.first == Swt::Operation::SCReset) {
          std::cout << "SC Reset |" /* boost::blank here */ << std::endl;
        } else if (out.first == Swt::Operation::Wait) {
          std::cout << "Wait  | " << std::dec << boost::get<int>(out.second) << std::endl;
        } else if (out.first == Swt::Operation::Error) {
          std::cout << "Error | " << boost::get<std::string>(out.second) << std::endl;
        } else {
          std::cout << "Unknown operation" << std::endl;
        }
      }
    }

    if (mOptions.ic) {
      std::cout << "Running IC test" << std::endl;
      auto ic = Ic(roc::SerialId{ mOptions.serial, mOptions.endpoint }, mOptions.link);
      ic.scReset();

      std::cout << "Running Simple IC operations" << std::endl;
      try {
        ic.write(0x54, 0xff);
        std::cout << ic.read(0x54) << std::endl;
        ic.write(0x55, 0xff);
        std::cout << ic.read(0x55) << std::endl;
        ic.write(0x56, 0xff);
        std::cout << ic.read(0x56) << std::endl;
      } catch (const IcException& e) {
        std::cerr << e.what() << std::endl;
      }

      std::cout << "Running an IC sequence" << std::endl;
      std::vector<std::pair<Ic::Operation, Ic::Data>> ops;
      ops.push_back({ Ic::Operation::Write, Ic::IcData{ 0x54, 0xff } });
      ops.push_back({ Ic::Operation::Read, Ic::IcData{ 0x54 } });
      ops.push_back({ Ic::Operation::Write, Ic::IcData{ 0x55, 0xff } });
      ops.push_back({ Ic::Operation::Read, Ic::IcData{ 0x55 } });
      ops.push_back({ Ic::Operation::Write, Ic::IcData{ 0x56, 0xff } });
      ops.push_back({ Ic::Operation::Read, Ic::IcData{ 0x56 } });
      auto output = ic.executeSequence(ops);
      for (const auto& out : output) {
        if (out.first == Ic::Operation::Write) {
          Ic::IcOut icOut = boost::get<Ic::IcOut>(out.second);
          std::cout << "Write | " << Util::formatValue(icOut) << std::endl;
        } else if (out.first == Ic::Operation::Read) {
          Ic::IcOut icOut = boost::get<Ic::IcOut>(out.second);
          std::cout << "Read | " << Util::formatValue(icOut) << std::endl;
        } else if (out.first == Ic::Operation::Error) {
          std::cout << "Error | " << boost::get<std::string>(out.second) << std::endl;
        }
      }
    }

    std::cout << "Exiting..." << std::endl;
  }

 private:
  struct OptionsStruct {
    int link = -1;
    bool ic = false;
    bool lla = false;
    bool sca = false;
    bool swt = false;
    int serial = -1;
    int endpoint = 0;
  } mOptions;
};

} // namespace alf
} // namespace o2

int main(int argc, char** argv)
{
  return o2::alf::AlfLibClient().execute(argc, argv);
}
