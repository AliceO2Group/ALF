/// \file Exception.h
/// \brief Definition of the ALF exceptions and related functions.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_ALF_ALF_EXCEPTION_H_
#define ALICEO2_ALF_ALF_EXCEPTION_H_

#include <Common/Exceptions.h>

namespace AliceO2 {
namespace Alf {

struct AlfException : AliceO2::Common::Exception {};
struct ScaException : AliceO2::Common::Exception {};
struct SwtException : AliceO2::Common::Exception {};

namespace ErrorInfo
{
using Message = AliceO2::Common::ErrorInfo::Message;
} // namespace ErrorInfo

} // namespace Alf
} // namespace AliceO2

#endif // ALICEO2_ALF_ALF_EXCEPTION_H_
