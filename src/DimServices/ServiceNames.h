/// \file ServiceNames.h
/// \brief Definition of DIM service names
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_ALF_SRC_DIMSERVICES_SERVICENAMES_H
#define ALICEO2_ALF_SRC_DIMSERVICES_SERVICENAMES_H

#include <string>
#include "Common.h"

namespace AliceO2
{
namespace Alf
{

class ServiceNames
{
  public:
    ServiceNames(AlfLink link)
        : mAlfId(std::to_string(link.alfId)), mSerial(link.serial), mLink(link.linkId)
    {
    }

    std::string registerRead() const;
    std::string registerWrite() const;
    std::string scaWrite() const;
    std::string scaSequence() const;
    std::string swtSequence() const;
    std::string publishRegistersStart() const;
    std::string publishRegistersStop() const;
    std::string publishScaSequenceStart() const;
    std::string publishScaSequenceStop() const;
    std::string publishSwtSequenceStart() const;
    std::string publishSwtSequenceStop() const;
    
    std::string publishRegistersSubdir(std::string name) const;
    std::string publishScaSequenceSubdir(std::string name) const;
    std::string publishSwtSequenceSubdir(std::string name) const;

  private:
    std::string format(std::string name) const;
    std::string mAlfId;
    const int mSerial;
    const int mLink;
};

} // namespace Alf
} // namespace AliceO2

#endif // ALICEO2_ALF_SRC_DIMSERVICES_SERVICENAMES_H
