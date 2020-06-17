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

#include "Lla/Lla.h"
#include "ReadoutCard/PatternPlayer.h"

namespace roc = AliceO2::roc;
namespace lla = o2::lla;

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
  static std::string patternPlayer(const std::string& parameter, std::shared_ptr<roc::BarInterface>);
  static std::string registerBlobWrite(const std::string& parameter, AlfLink link);
  std::string llaSessionStart(const std::string& parameter, int cardSequence);
  std::string llaSessionStop(const std::string& parameter, int cardSequence);

  static std::vector<uint32_t> stringToRegisterPair(const std::string stringPair);
  static std::pair<Sca::Data, Sca::Operation> stringToScaPair(const std::string stringPair);
  static std::pair<Swt::Data, Swt::Operation> stringToSwtPair(const std::string stringPair);
  static std::pair<Ic::IcData, Ic::Operation> stringToIcPair(const std::string stringPair);
  static std::vector<std::vector<uint32_t>> parseStringToRegisterPairs(std::vector<std::string> stringPairs);
  static std::vector<std::pair<Sca::Data, Sca::Operation>> parseStringToScaPairs(std::vector<std::string> stringPairs);
  static std::vector<std::pair<Swt::Data, Swt::Operation>> parseStringToSwtPairs(std::vector<std::string> stringPairs);
  static std::vector<std::pair<Ic::IcData, Ic::Operation>> parseStringToIcPairs(std::vector<std::string> stringPairs);
  static roc::PatternPlayer::Info parseStringToPatternPlayerInfo(const std::vector<std::string> sringsPairs);

  /// cardSequence -> link -> vector of RPC servers
  std::map<int, std::map<int, std::vector<std::unique_ptr<StringRpcServer>>>> mRpcServers;

  std::map<int, std::unique_ptr<lla::Session>> mSessions;
};

} // namespace Alf
} // namespace AliceO2

#endif // ALICEO2_ALF_ALFSERVER_H_
