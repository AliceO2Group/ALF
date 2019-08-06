/// \file ServiceNames.cxx
/// \brief Implementation of DIM service names
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "DimServices/ServiceNames.h"
#include <boost/format.hpp>

namespace AliceO2
{
namespace Alf
{

#define DEFSERVICENAME(_function, _name) \
std::string ServiceNames::_function() const \
{ \
  return format(_name); \
}

DEFSERVICENAME(registerRead, "REGISTER_READ")
DEFSERVICENAME(registerWrite, "REGISTER_WRITE")
DEFSERVICENAME(scaSequence, "SCA_SEQUENCE")
DEFSERVICENAME(swtSequence, "SWT_SEQUENCE")
DEFSERVICENAME(publishRegistersStart, "PUBLISH_REGISTERS_START")
DEFSERVICENAME(publishRegistersStop, "PUBLISH_REGISTERS_STOP")
DEFSERVICENAME(publishScaSequenceStart, "PUBLISH_SCA_SEQUENCE_START")
DEFSERVICENAME(publishScaSequenceStop, "PUBLISH_SCA_SEQUENCE_STOP")
DEFSERVICENAME(publishSwtSequenceStart, "PUBLISH_SWT_SEQUENCE_START")
DEFSERVICENAME(publishSwtSequenceStop, "PUBLISH_SWT_SEQUENCE_STOP")

std::string ServiceNames::format(std::string name) const
{
  if (mAlfId != "-1")
    return ((boost::format("ALF%1%/serial_%2%/link_%3%/%4%") % mAlfId % mSerial % mLink % name)).str();
  else
    return ((boost::format("ALF/serial_%1%/link_%2%/%3%") % mSerial % mLink % name)).str();
}

std::string ServiceNames::publishRegistersSubdir(std::string name) const
{
  return ServiceNames::format("PUBLISH_REGISTERS/") + name;
}

std::string ServiceNames::publishScaSequenceSubdir(std::string name) const
{
  return ServiceNames::format("PUBLISH_SCA_SEQUENCE/") + name;
}

std::string ServiceNames::publishSwtSequenceSubdir(std::string name) const
{
  return ServiceNames::format("PUBLISH_SWT_SEQUENCE/") + name;
}


} // namespace Alf
} // namespace AliceO2
