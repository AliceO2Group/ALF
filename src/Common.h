#ifndef ALICEO2_ALF_COMMON_H_
#define ALICEO2_ALF_COMMON_H_

#include <chrono>
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

} // namespace AliceO2
} // namespace Alf

#endif // ALICEO2_ALF_COMMON_H_
