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

/// \file SwtWord.h
/// \brief Definition of the SwtWord class
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef ALICEO2_ALF_SRC_SWT_SWTWORD_H
#define ALICEO2_ALF_SRC_SWT_SWTWORD_H

namespace AliceO2
{
namespace Alf
{

class SwtWord
{
 public:
  SwtWord();
  SwtWord(uint32_t low, uint32_t med, uint16_t high);
  SwtWord(uint64_t swtInt);

  bool operator==(const SwtWord& swtWord);
  bool operator!=(const SwtWord& swtWord);
  void setLow(uint32_t low);
  void setMed(uint32_t med);
  void setHigh(uint16_t high);
  uint64_t getLow() const;
  uint64_t getMed() const;
  uint16_t getHigh() const;

  enum Size {
    High,
    Medium,
    Low
  };

 private:
  uint64_t mLow;
  uint64_t mMed;
  uint16_t mHigh;
};

std::ostream& operator<<(std::ostream& output, const SwtWord& swtWord);

} // namespace Alf
} // namespace AliceO2

#endif // ALICEO2_ALF_SRC_SWT_SWTWORD_H
