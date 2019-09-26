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

#include <boost/algorithm/string/predicate.hpp>
#include <chrono>
#include <iomanip>
#include <thread>

#include "AlfException.h"
#include "DimServices/DimServices.h"
#include "Common.h"
#include "Ic/Ic.h"

namespace roc = AliceO2::roc;

namespace AliceO2
{
namespace Alf
{

class AlfServer
{
 public:
  AlfServer();
  void makeRpcServers(std::vector<AlfLink> links);

 private:
  static std::string registerRead(const std::string& parameter, std::shared_ptr<roc::BarInterface>);
  static std::string registerWrite(const std::string& parameter, std::shared_ptr<roc::BarInterface>);
  static std::string scaBlobWrite(const std::string& parameter, AlfLink link);
  static std::string swtBlobWrite(const std::string& parameter, AlfLink link);
  static std::string icBlobWrite(const std::string& parameter, AlfLink link);
  static std::string icGbtI2cWrite(const std::string& parameter, AlfLink link);

  static Sca::CommandData stringToScaPair(std::string stringPair);
  static std::pair<Swt::SwtData, Swt::Operation> stringToSwtPair(const std::string stringPair);
  static std::pair<Ic::IcData, Ic::Operation> stringToIcPair(const std::string stringPair);
  static std::vector<Sca::CommandData> parseStringToScaCommands(std::vector<std::string> stringPairs);
  static std::vector<std::pair<Swt::SwtData, Swt::Operation>> parseStringToSwtPairs(std::vector<std::string> stringPairs);
  static std::vector<std::pair<Ic::IcData, Ic::Operation>> parseStringToIcPairs(std::vector<std::string> stringPairs);

  /// serial -> link -> vector of RPC servers
  std::map<int, std::map<int, std::vector<std::unique_ptr<StringRpcServer>>>> mRpcServers;
};

} // namespace Alf
} // namespace AliceO2

#endif // ALICEO2_ALF_ALFSERVER_H_
