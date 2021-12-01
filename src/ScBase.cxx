
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

/// \file ScBase.cxx
/// \brief Implementation of ALICE Lowlevel Frontend (ALF) SC base class
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <boost/format.hpp>
#include <chrono>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>

#include "ReadoutCard/CardFinder.h"
#include "ReadoutCard/ChannelFactory.h"
#include "ReadoutCard/Cru.h"

#include "Alf/Exception.h"
#include "Alf/ScBase.h"

#include "Logger.h"
#include "Util.h"

// hijacking this file to define an extern variable for the Logger
bool kDebugLogging;

namespace sc_regs = AliceO2::roc::Cru::ScRegisters;

namespace o2
{
namespace alf
{

ScBase::ScBase(AlfLink link, std::shared_ptr<lla::Session> llaSession)
  : mLink(link), mBar2(link.bar)
{
  mLlaSession = std::make_unique<LlaSession>(llaSession);
}

ScBase::ScBase(const roc::Parameters::CardIdType& cardId, int linkId)
{
  init(cardId, linkId);
}

ScBase::ScBase(std::string cardId, int linkId)
{
  init(roc::Parameters::cardIdFromString(cardId), linkId);
}

void ScBase::init(const roc::Parameters::CardIdType& cardId, int linkId)
{
  if (linkId >= kCruNumLinks) {
    BOOST_THROW_EXCEPTION(
      ScException() << ErrorInfo::Message("Maximum link number exceeded"));
  }

  auto card = roc::findCard(cardId);
  mBar2 = roc::ChannelFactory().getBar(cardId, 2);

  mLink = AlfLink{
    "DDT",
    card.serialId,
    linkId,
    card.serialId.getEndpoint() * kCruNumLinks + linkId,
    mBar2,
    roc::CardType::Cru
  };

  mLlaSession = std::make_unique<LlaSession>("DDT", card.serialId);
}

void ScBase::setChannel(int gbtChannel)
{
  if (gbtChannel >= kCruNumLinks) {
    BOOST_THROW_EXCEPTION(
      ScException() << ErrorInfo::Message("Maximum link number exceeded"));
  }

  mLink.linkId = gbtChannel;
  mLink.rawLinkId = mLink.serialId.getEndpoint() * kCruNumLinks + gbtChannel;
}

void ScBase::checkChannelSet()
{
  if (mLink.linkId == -1) {
    BOOST_THROW_EXCEPTION(ScException() << ErrorInfo::Message("No channel selected"));
  }

  setChannel(mLink.linkId);
}

void ScBase::scReset()
{
  barWrite(sc_regs::SC_RESET.index, 0x1);
  barWrite(sc_regs::SC_RESET.index, 0x0); //void cmd to sync clocks
}

void ScBase::barWrite(uint32_t index, uint32_t data)
{
  uint32_t linkIndex = (0x00f00000 + (mLink.rawLinkId << 8)) / 4 + index;
  mBar2->writeRegister(linkIndex, data);
}

uint32_t ScBase::barRead(uint32_t index)
{
  uint32_t linkIndex = (0x00f00000 + (mLink.rawLinkId << 8)) / 4 + index;
  return mBar2->readRegister(linkIndex);
}

} // namespace alf
} // namespace o2
