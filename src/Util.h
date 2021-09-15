
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

/// \file Util.h
/// \brief Convenience functions for the need of the project
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef O2_ALF_SRC_UTIL_H
#define O2_ALF_SRC_UTIL_H

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

#include "Alf/Common.h"
#include "Logger.h"

namespace o2
{
namespace alf
{
namespace Util
{

template <typename T>
T getBit(T x, int index)
{
  return (x >> index) & 0x1;
}

inline uint32_t stringToHex(const std::string& string)
{
  uint64_t n = std::stoul(string, nullptr, 16);
  if (n > std::numeric_limits<uint32_t>::max()) {
    BOOST_THROW_EXCEPTION(std::out_of_range("Parameter does not fit in 32-bit unsigned int"));
  }
  return n;
}

inline void checkAddress(uint64_t address)
{
  if (address < 0x1e8 || address > 0x1fc) { //TODO: Update these addresses (Checkers for SWT, SCA and BAR2 regs)
    BOOST_THROW_EXCEPTION(std::out_of_range("Address out of range"));
  }
}

inline std::string formatValue(uint32_t value)
{
  return (boost::format("0x%08x") % value).str();
}

inline std::vector<std::string> split(const std::string& input, std::string separators) //TODO: Does split throw?
{
  std::vector<std::string> output;
  boost::split(output, input, boost::is_any_of(separators.c_str()));
  return output;
}

inline size_t strlenMax(char* str, size_t max)
{
  for (size_t i = 0; i < max; i++) {
    if (str[i] == '\0') {
      return i;
    }
  }
  return max;
}

} // namespace Util
} // namespace alf
} // namespace o2

#endif // O2_ALF_SRC_UTIL_H
