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
/// \brief Definition of SWT operations
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef ALICEO2_ALF_SRC_SWT_SWT_H
#define ALICEO2_ALF_SRC_SWT_SWT_H

#include <string>
#include <variant>

#include "Common.h"
#include "ReadoutCard/RegisterReadWriteInterface.h"
#include "Swt/SwtWord.h"

namespace roc = AliceO2::roc;

namespace AliceO2
{
namespace Alf
{

/// Class for Single Word Transactions with the CRU
class Swt
{
 public:
  Swt(AlfLink link);

  typedef int TimeOut;
  typedef std::variant<SwtWord, TimeOut> SwtData;

  void reset();
  uint32_t write(const SwtWord& swtWord);
  void read(std::vector<SwtWord>& word, TimeOut msTimeOut = 10, SwtWord::Size wordSize = SwtWord::Size::High);

  enum Operation { Read,
                   Write,
                   Reset };

  std::string writeSequence(std::vector<std::pair<SwtData, Operation>> sequence);

 private:
  void setChannel(int gbtChannel);
  void barWrite(uint32_t offset, uint32_t data);
  uint32_t barRead(uint32_t index);

  roc::RegisterReadWriteInterface& mBar2;

  AlfLink mLink;
};

} // namespace Alf
} // namespace AliceO2

#endif // ALICEO2_ALF_SRC_SWT_SWT_H
