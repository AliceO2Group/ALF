#ifndef ALICEO2_ALF_COMMON_H_
#define ALICEO2_ALF_COMMON_H_

#include <chrono>
#include "InfoLogger/InfoLogger.hxx"
#include "ReadoutCard/BarInterface.h"

namespace AliceO2
{
namespace Alf
{

static constexpr int CRU_NUM_LINKS(24);
static constexpr int CRORC_NUM_LINKS(6);

static constexpr auto BUSY_TIMEOUT = std::chrono::milliseconds(10);
static constexpr auto CHANNEL_BUSY_TIMEOUT = std::chrono::milliseconds(10);

struct AlfLink {
  int alfId;
  int serial;
  int linkId;
  std::shared_ptr<roc::BarInterface> bar2; 
};

constexpr auto endm = AliceO2::InfoLogger::InfoLogger::endm;

static AliceO2::InfoLogger::InfoLogger& getLogger()
{
  static AliceO2::InfoLogger::InfoLogger logger;
  return logger;
}


} // namespace AliceO2
} // namespace Alf

#endif // ALICEO2_ALF_COMMON_H_
