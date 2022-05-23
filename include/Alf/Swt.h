
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

// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file Swt.h
/// \brief Definition of ALICE Lowlevel Frontend (ALF) SWT operations
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef O2_ALF_INC_SWT_H
#define O2_ALF_INC_SWT_H

#include <string>
#include <boost/blank.hpp>
#include <boost/variant.hpp>

#include "ReadoutCard/BarInterface.h"
#include "ReadoutCard/Parameters.h"

#include "Common.h"
#include "Alf/Lla.h"
#include "Alf/ScBase.h"
#include "Alf/SwtWord.h"

namespace roc = AliceO2::roc;

namespace o2
{
namespace alf
{

/// Class for Single Word Transactions with the CRU
class Swt : public ScBase
{
 public:
  typedef int TimeOut, WaitTime;

  /// Typedef for the Data type of an SWT sequence operation.
  /// Variant of TimeOut for reads, SwtWord for writes, std::string for Errors
  typedef boost::variant<boost::blank, TimeOut, WaitTime, SwtWord, std::string> Data;

  /// Enum for the different SWT operation types
  enum Operation { Read,
                   Write,
                   SCReset,
                   Wait,
                   Error,
                   Lock };

  /// Internal constructor for the ALF server
  /// \param link AlfLink holding useful information coming from the AlfServer class
  Swt(AlfLink link, std::shared_ptr<lla::Session> llaSession, SwtWord::Size = SwtWord::Size::Low);

  /// External constructor
  /// \param cardId The card ID for which to get the SWT handle.
  /// \param linkId The link ID to set the channel to (optional).
  Swt(const roc::Parameters::CardIdType& cardId, int linkId = -1);

  /// External constructor
  /// \param cardId The card ID for which to get the SWT handle.
  /// \param linkId The link ID to set the channel to (optional).
  Swt(std::string cardId, int linkId = -1);

  /// Writes an SWT word
  /// \param swtWord The SWT word to write
  void write(const SwtWord& swtWord);

  /// Reads SWT words
  /// \param wordSize The size of the SWT words to be read
  /// \param msTimeOut Timeout of the read operation in ms
  /// \return A vector of SWT words read
  /// \throws o2::alf::SwtException in case of no SWT words in FIFO, or timeout exceeded
  std::vector<SwtWord> read(SwtWord::Size wordSize = SwtWord::Size::Low, TimeOut msTimeOut = DEFAULT_SWT_TIMEOUT_MS);

  /// Executes an SWT sequence
  /// \param sequence A vector of Operation and Data pairs
  /// \return A vector of Operation and resulting Data pairs
  ///         Write -> Echoes written data
  ///         Read  -> The SwtWord read
  ///         Reset -> Empty Data
  ///         Error -> Error message in std::string
  /// \throws o2:lla::LlaException on lock fail
  std::vector<std::pair<Operation, Data>> executeSequence(std::vector<std::pair<Operation, Data>> sequence, bool lock = false);

  /// Executes an SWT sequence for the ALF server
  /// \param sequence A vector of Data and Operation pairs
  /// \return A string of newline separated results;
  /// \throws o2:lla::LlaException on lock fail
  ///         o2::alf::SwtException on invalid operation or error
  std::string writeSequence(std::vector<std::pair<Operation, Data>> sequence, bool lock = false);

  static std::string SwtOperationToString(Operation op);
  static Operation StringToSwtOperation(std::string op);

  static constexpr int DEFAULT_SWT_TIMEOUT_MS = 10;

 private:
  static constexpr int DEFAULT_SWT_WAIT_TIME_MS = 3;
  SwtWord::Size mSwtWordSize = SwtWord::Size::Low;
};

} // namespace alf
} // namespace o2

#endif // O2_ALF_INC_SWT_H
