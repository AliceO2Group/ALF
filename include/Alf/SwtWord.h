
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

/// \file SwtWord.h
/// \brief Definition of the SwtWord class
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef O2_ALF_SRC_SWT_SWTWORD_H
#define O2_ALF_SRC_SWT_SWTWORD_H

namespace o2
{
namespace alf
{

class SwtWord
{
 public:
  enum Size {
    High,
    Medium,
    Low
  };

  SwtWord(SwtWord::Size size = SwtWord::Size::Low);
  SwtWord(uint32_t low, uint32_t med, uint16_t high, SwtWord::Size size = SwtWord::Size::Low);
  SwtWord(uint64_t swtInt, SwtWord::Size size = SwtWord::Size::Low);

  bool operator==(const SwtWord& swtWord);
  bool operator!=(const SwtWord& swtWord);
  void setLow(uint32_t low);
  void setMed(uint32_t med);
  void setHigh(uint16_t high);
  void setSize(Size size);
  void setSize(std::string size);
  uint32_t getLow() const;
  uint32_t getMed() const;
  uint16_t getHigh() const;
  Size getSize() const;

  static SwtWord::Size sizeFromString(std::string swtWord);

 private:
  uint32_t mLow;
  uint32_t mMed;
  uint16_t mHigh;
  Size mSize;
};

std::ostream& operator<<(std::ostream& output, const SwtWord& swtWord);

} // namespace alf
} // namespace o2

#endif // O2_ALF_SRC_SWT_SWTWORD_H
