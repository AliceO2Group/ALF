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
    std::string scaSequence() const;
    std::string swtSequence() const;
    std::string publishRegistersStart() const;
    std::string publishRegistersStop() const;
    std::string publishScaSequenceStart() const;
    std::string publishScaSequenceStop() const;
    std::string publishSwtSequenceStart() const;
    std::string publishSwtSequenceStop() const;
    
    std::string publishRegisters(std::string name) const;
    std::string publishScaSequence(std::string name) const;
    std::string publishSwtSequence(std::string name) const;

    static std::string getTail(std::string name);

  private:
    std::string format(std::string name) const;
    std::string mAlfId;
    const int mSerial;
    const int mLink;
};

} // namespace Alf
} // namespace AliceO2

#endif // ALICEO2_ALF_SRC_DIMSERVICES_SERVICENAMES_H
