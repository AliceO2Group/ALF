#ifndef ALICEO2_ALF_SRC_UTIL_H
#define ALICEO2_ALF_SRC_UTIL_H

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

namespace AliceO2
{
namespace Alf
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
  if (address < 0x1e8 || address > 0x1fc && false) { //TODO: Update these addresses after testing!!
    BOOST_THROW_EXCEPTION(std::out_of_range("Address out of range")); //TODO: Print an error and return, do NOT crash
  }
}

inline std::string formatValue(uint32_t value)
{
  return (boost::format("0x%08x") % value).str();
}

inline std::vector<std::string> split(const std::string& input, std::string separators)
{
  std::vector<std::string> output;
  boost::split(output, input, boost::is_any_of(separators.c_str()));
  return output;
}

inline size_t strlenMax(char * str, size_t max)
{
  for (size_t i = 0; i < max; i++) {
    if (str[i] == '\0') {
      return i;
    }
  }
  return max;
}

}
} // namespace Alf
} // namespace AliceO2

#endif // ALICEO2_ALF_SRC_UTIL_H
