
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

/// \file IcPythonInterface.cxx
/// \brief Python IC interface for the Slow Control library
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef O2_ALF_SRC_ICPYTHONINTERFACE_H
#define O2_ALF_SRC_ICPYTHONINTERFACE_H

#include <boost/python.hpp>
#include "Alf/Ic.h"
#include "PythonInterface.h"

namespace o2
{
namespace alf
{
namespace python
{

namespace bp = boost::python;
typedef boost::variant<std::string, std::pair<std::string, uint32_t>, std::pair<std::string, std::pair<uint32_t, uint32_t>>> IcArgsVariant;

/// Exposed python function docs
auto sIcInitDoc =
  R"(Initializes an IC object

Args:
    card id: String containing PCI address (e.g. 42:0.0), serial-endpoint pair (e.g. 10241:1), sequence number (e.g. #2)
    channel: Channel number to initially set)";

auto sIcSetChannelDoc =
  R"(Sets the Slow Control channel

Args:
    channel: Channel number to set)";

auto sIcScResetDoc =
  R"(Resets Slow Control)";

auto sIcWriteDoc =
  R"(Perform an IC write
  
Args:
    address: IC address (unsigned 32-bit) to write to
    data: IC data (unsigned 32-bit))";

auto sIcReadDoc =
  R"(Perform an IC write
  
Args:
    address: IC address (unsigned 32-bit) to read from)
             
Returns:
    data: IC data (unsigned 32-bit) read)";

auto sIcWriteGbtI2cDoc =
  R"(Perform an IC GBT I2C write
  
Args:
    data: Data (unsigned 32-bit) to write)";

auto sIcSequenceDoc =
  R"(Execute an IC sequence:

Args:
  sequence: A list of tuples made up of:
    operation: An operation(string) to perform
    input: Input((unsigned 32-bit int, unsigned 32-bit int), unsigned 32-bit int) to said operation

      Operations and inputs:
        operation: "sc_reset"
        input: no-input

        operation: "read"
        input: address (unsigned 32-bit int)

        operation: "write"
        input: address, data tuple (unsigned 32-bit int, unsigned 32-bit int)
    
  lock: boolean to execute the sequence within an LLA session

Returns:
  sequence: A list of tuples made up of: 
    operation: The operation carried out (string, same as input)
    output: Output (string, (unsigned 32-bit int, unsigned 32-bit int), unsigned 32-bit int) of said operation)
  
      Output per operation:
        operation: "sc_reset"
        output: "" (string)

        operation: "read"
        output: data (unsigned 32-bit int)

        operation: "write"
        output: echo of the address and data tuple (unsigned 32-bit int, unsigned 32-bit int)
)";

struct IcArgsVariantConverter {
  IcArgsVariantConverter&
    fromPython()
  {
    bp::converter::registry::push_back(
      &IcArgsVariantConverter::convertible,
      &IcArgsVariantConverter::construct,
      bp::type_id<IcArgsVariant>());

    return *this;
  }

  static void* convertible(PyObject* obj)
  {
    return obj;
  }

  static void construct(PyObject* obj, bp::converter::rvalue_from_python_stage1_data* data)
  {
    bp::handle<> handle(bp::borrowed(obj));
    void* storage = ((bp::converter::rvalue_from_python_storage<IcArgsVariant>*)data)->storage.bytes;

    IcArgsVariant ret;

    bp::extract<std::string> s(obj);
    if (s.check()) {
      ret = s();
    } else {
      bp::tuple tuple(bp::borrowed(obj));
      bp::extract<uint32_t> i(tuple[1]);
      if (!i.check()) {
        ret = std::pair<std::string, std::pair<uint32_t, uint32_t>>(bp::extract<std::string>(tuple[0]),
                                                                    bp::extract<std::pair<uint32_t, uint32_t>>(tuple[1]));
      } else {
        ret = bp::extract<std::pair<std::string, uint32_t>>(tuple);
      }
    }

    new (storage) IcArgsVariant(ret);
    data->convertible = storage;
    return;
  }
};

class IcInterface
{
 public:
  IcInterface(std::string cardIdString, int linkId = -1)
  {
    mIc = std::make_shared<Ic>(cardIdString, linkId);
  }

  void setChannel(int gbtChannel)
  {
    mIc->setChannel(gbtChannel);
  }

  void scReset()
  {
    mIc->scReset();
  }

  uint32_t read(uint32_t address)
  {
    return mIc->read(address);
  }

  void write(uint32_t address, uint32_t data)
  {
    mIc->write(address, data);
  }

  void writeGbtI2c(uint32_t data)
  {
    mIc->writeGbtI2c(data);
  }

  std::vector<std::pair<Ic::Operation, Ic::Data>> sequence(std::vector<IcArgsVariant> sequence, bool lock = false)
  {
    ScopedGILRelease s; // enable boost::python multi-threading
    std::vector<std::pair<Ic::Operation, Ic::Data>> icSequence;
    for (const auto& v : sequence) {
      icSequence.push_back(boost::apply_visitor(IcArgsVariantVisitor(), v));
    }
    return mIc->executeSequence(icSequence, lock);
  }

  std::vector<std::pair<Ic::Operation, Ic::Data>> sequenceDefault(std::vector<IcArgsVariant> icSeq)
  {
    return sequence(icSeq);
  }

 private:
  struct IcArgsVariantVisitor
    : public boost::static_visitor<std::pair<Ic::Operation, Ic::Data>> {
    auto operator()(std::string v) const
    {
      return std::make_pair(Ic::StringToIcOperation(v), 0);
    }

    auto operator()(std::pair<std::string, uint32_t> v) const
    {
      return std::make_pair(Ic::StringToIcOperation(v.first), v.second);
    }

    auto operator()(std::pair<std::string, std::pair<uint32_t, uint32_t>> v) const
    {
      return std::make_pair(Ic::StringToIcOperation(v.first), Ic::IcData{ v.second.first, v.second.second });
    }
  };

  std::shared_ptr<Ic> mIc;
};

struct IcOperationDataVectorToPythonListOfTuples {
  static PyObject* convert(const std::vector<std::pair<Ic::Operation, Ic::Data>>& seqOut)
  {
    bp::list ret;
    for (const auto& el : seqOut) {
      ret.append(bp::make_tuple(Ic::IcOperationToString(el.first), el.second));
    }
    return bp::incref(bp::object(ret).ptr());
  }
};

struct IcDataToPython {
  struct IcDataConvertVisitor : public boost::static_visitor<PyObject*> {
    auto operator()(Ic::IcData data) const
    {
      bp::tuple ret = bp::make_tuple(data.address, data.data);
      return bp::incref(bp::object(ret).ptr());
    }

    auto operator()(int data) const
    {
      return bp::incref(bp::object(data).ptr());
    }

    auto operator()(std::string data) const
    {
      return bp::incref(bp::object(data).ptr());
    }
  };

  static PyObject* convert(Ic::Data data)
  {
    return boost::apply_visitor(IcDataConvertVisitor(), data);
  }
};

} //namespace python
} //namespace alf
} //namespace o2

#endif // O2_ALF_SRC_ICPYTHONINTERFACE_H
