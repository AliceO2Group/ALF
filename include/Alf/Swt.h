// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
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

#include "Common.h"
#include "ReadoutCard/BarInterface.h"
#include "ReadoutCard/Parameters.h"
#include "Alf/SwtWord.h"

namespace roc = AliceO2::roc;

namespace o2
{
namespace alf
{

/// Class for Single Word Transactions with the CRU
class Swt
{
 public:
  typedef int TimeOut;

  /// Typedef for the Data type of an SWT sequence operation.
  /// Variant of TimeOut for reads, SwtWord for writes, std::string for Errors; useful for DIM RPCs
  typedef boost::variant<boost::blank, TimeOut, SwtWord, std::string> Data;

  /// Enum for the different SWT operation types
  enum Operation { Read,
                   Write,
                   Reset,
                   Error };

  /// Internal constructor for the ALF server
  /// \param link AlfLink holding useful information coming from the AlfServer class
  Swt(AlfLink link);

  /// External constructor
  /// \param cardId The card ID for which to get the SWT handle.
  /// \param linkId The link ID for which to get the SWT handle.
  Swt(const roc::Parameters::CardIdType& cardId, int linkId);

  /// External constructor
  /// \param cardId The card ID for which to get the SWT handle.
  /// \param linkId The link ID for which to get the SWT handle.
  Swt(std::string cardId, int linkId);

  /// Resets the SWT channel selected
  void reset();

  /// Writes an SWT word
  /// \param swtWord The SWT word to write
  /// \param wordSize The size of the SWT word to be written
  void write(const SwtWord& swtWord);

  /// Reads SWT words
  /// \param wordSize The size of the SWT words to be read
  /// \param msTimeOut Timeout of the read operation in ms
  /// \return A vector of SWT words read
  /// \throws o2::alf::SwtException in case of no SWT words in FIFO, or timeout exceeded
  std::vector<SwtWord> read(SwtWord::Size wordSize = SwtWord::Size::High, TimeOut msTimeOut = DEFAULT_SWT_TIMEOUT_MS);

  /// Executes an SWT sequence
  /// \param sequence A vector of Operation and Data pairs
  /// \return A vector of Operation and resulting Data pairs
  //   Write -> Echoes written data
  //   Read  -> The SwtWord read
  //   Reset -> Empty Data
  //   Error -> Error message in std::string
  std::vector<std::pair<Operation, Data>> executeSequence(std::vector<std::pair<Operation, Data>> sequence);

  /// Executes an SWT sequence for the ALF server
  /// \param sequence A vector of Data and Operation pairs
  /// \return A string of newline separated results;
  ///  0 for successful writes, the SwtWord for succesful reads
  /// \throws o2::alf::SwtException on invalid operation or error
  std::string writeSequence(std::vector<std::pair<Operation, Data>> sequence);

 private:
  void init(const roc::Parameters::CardIdType& cardId, int linkId);
  void setChannel(int gbtChannel);
  void barWrite(uint32_t offset, uint32_t data);
  uint32_t barRead(uint32_t index);

  std::shared_ptr<roc::BarInterface> mBar2;

  AlfLink mLink;
  int mWordSequence = 0; //start from 0, as after the reset the counter starts at 1
  static constexpr int DEFAULT_SWT_TIMEOUT_MS = 10;
};

} // namespace alf
} // namespace o2

#endif // O2_ALF_INC_SWT_H
