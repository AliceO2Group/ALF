
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

/// \file example.cxx
/// \brief An example on the use of ALF as a library
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <chrono>
#include <thread>
#include <vector>

#include "Alf/Alf.h"

using namespace o2::alf;

int main()
{

  std::cout << "Running SWT test" << std::endl;
  std::string cardId("#2");
  auto swt = Swt(cardId, 0); // Get an SWT handle for card #2 on channel 0

  std::cout << "Running simple SWT operations" << std::endl;
  try {
    swt.reset();
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
  swt.setChannel(1); // Change the channel to 1
  ops.push_back({ Swt::Operation::Reset, {} });
  ops.push_back({ Swt::Operation::Write, SwtWord{ 0xcafe, 0x41d, 0x0 } });
  ops.push_back({ Swt::Operation::Write, SwtWord{ 0xb00f, 0x42, 0x88, SwtWord::Size::High } });
  ops.push_back({ Swt::Operation::Write, SwtWord{ 0xb00f, 0x42, 0x88 } });
  ops.push_back({ Swt::Operation::Read, { 50 } });
  ops.push_back({ Swt::Operation::Error, {} }); // inject error
  auto output = swt.executeSequence(ops, true); // execute the sequence atomically
  for (const auto& out : output) {
    if (out.first == Swt::Operation::Write) {
      std::cout << "Write | " << boost::get<SwtWord>(out.second) << std::endl;
    } else if (out.first == Swt::Operation::Read) {
      std::cout << "Read  | " << boost::get<SwtWord>(out.second) << std::endl;
    } else if (out.first == Swt::Operation::Reset) {
      std::cout << "Reset |" << std::endl;
    } else if (out.first == Swt::Operation::Error) {
      std::cout << "Error | " << boost::get<std::string>(out.second) << std::endl;
    } else {
      std::cout << "Unknown operation" << std::endl;
    }
  }

  return 0;
}
