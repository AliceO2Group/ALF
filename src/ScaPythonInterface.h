
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

/// \file ScaPythonInterface.cxx
/// \brief Python SCA interface for the Slow Control library
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef O2_ALF_SRC_SCAPYTHONINTERFACE_H
#define O2_ALF_SRC_SCAPYTHONINTERFACE_H

#include <boost/python.hpp>
#include "Alf/Sca.h"
#include "PythonInterface.h"

namespace o2
{
namespace alf
{
namespace python
{

namespace bp = boost::python;
typedef boost::variant<std::string, std::pair<std::string, uint32_t>, std::pair<std::string, std::pair<uint32_t, uint32_t>>> ScaArgsVariant;

/// Exposed python function docs
auto sScaInitDoc =
  R"(Initializes an Swt object

Args:
    card id: String containing PCI address (e.g. 42:0.0), serial-endpoint pair (e.g. 10241:1), sequence number (e.g. #2)
    channel: Channel number to initially set)";

auto sScaSetChannelDoc =
  R"(Sets the Slow Control channel

Args:
    channel: Channel number to set)";

auto sScaScResetDoc =
  R"(Resets Slow Control)";

auto sScaSvlResetDoc =
  R"(Performs SVL reset)";

auto sScaSvlConnectDoc =
  R"(Performs SVL connect)";

auto sScaExecuteCommandDoc =
  R"(Execute an SCA command
  
Args:
    command: SCA command (unsigned 32-bit int) to execute
    data: SCA data (unsigned 32-bit int))";

auto sScaSequenceDoc =
  R"(Execute an SCA sequence:

Args:
  sequence: A list of tuples made up of:
    operation: An operation(string) to perform
    input: Input((unsigned 32-bit int, unsigned 32-bit int), int) to said operation

      Operations and input types:
        operation: "sc_reset"
        input: no-input

        operation: "svl_reset"
        input: no-input

        operation: "svl_connect"
        input: no-input

        operation: "wait"
        input: wait_time (optional int)

        operation: "command"
        input: SCA command, data tuple (unsigned 32-bit int, unsigned 32-bit int)

  lock: boolean to execute the sequence within an LLA session

Returns:
  sequence: A list of tuples made up of: 
    operation: The operation carried out (string, same as input)
    output: Output (string, (unsigned 32-bit int, unsigned 32-bit int), int) of said operation)
  
      Output per operation:
        operation: "sc_reset"
        output: "" (string)

        operation: "svl_reset"
        output: "" (string)

        operation: "svl_connect"
        output: "" (string)

        operation: "wait"
        output: wait_time (int)

        operation: "command"
        output: a tuple of command and output (unsigned 32-bit int, unsigned 32-bit int)

)";

struct ScaArgsVariantConverter {
  ScaArgsVariantConverter&
    fromPython()
  {
    bp::converter::registry::push_back(
      &ScaArgsVariantConverter::convertible,
      &ScaArgsVariantConverter::construct,
      bp::type_id<ScaArgsVariant>());

    return *this;
  }

  static void* convertible(PyObject* obj)
  {
    return obj;
  }

  static void construct(PyObject* obj, bp::converter::rvalue_from_python_stage1_data* data)
  {
    bp::handle<> handle(bp::borrowed(obj));
    void* storage = ((bp::converter::rvalue_from_python_storage<ScaArgsVariant>*)data)->storage.bytes;

    ScaArgsVariant ret;

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

    new (storage) ScaArgsVariant(ret);
    data->convertible = storage;
    return;
  }
};

class ScaInterface
{
 public:
  ScaInterface(std::string cardIdString, int linkId = -1)
  {
    mSca = std::make_shared<Sca>(cardIdString, linkId);
  }

  void setChannel(int gbtChannel)
  {
    mSca->setChannel(gbtChannel);
  }

  void scReset()
  {
    mSca->scReset();
  }

  void svlReset()
  {
    mSca->svlReset();
  }

  void svlConnect()
  {
    mSca->svlConnect();
  }

  Sca::CommandData executeCommand(uint32_t cmd, uint32_t data)
  {
    return mSca->executeCommand(cmd, data);
  }

  std::vector<std::pair<Sca::Operation, Sca::Data>> sequence(std::vector<ScaArgsVariant> sequence, bool lock = false)
  {
    ScopedGILRelease s; // enable boost::python multi-threading
    std::vector<std::pair<Sca::Operation, Sca::Data>> scaSequence;
    for (const auto& v : sequence) {
      scaSequence.push_back(boost::apply_visitor(ScaArgsVariantVisitor(), v));
    }
    return mSca->executeSequence(scaSequence, lock);
  }

  std::vector<std::pair<Sca::Operation, Sca::Data>> sequenceDefault(std::vector<ScaArgsVariant> scaSeq)
  {
    return sequence(scaSeq);
  }

 private:
  struct ScaArgsVariantVisitor
    : public boost::static_visitor<std::pair<Sca::Operation, Sca::Data>> {
    auto operator()(std::string v) const
    {
      return std::make_pair(Sca::StringToScaOperation(v), 0);
    }

    auto operator()(std::pair<std::string, uint32_t> v) const
    {
      return std::make_pair(Sca::StringToScaOperation(v.first), v.second);
    }

    auto operator()(std::pair<std::string, std::pair<uint32_t, uint32_t>> v) const
    {
      return std::make_pair(Sca::StringToScaOperation(v.first), Sca::CommandData{ v.second.first, v.second.second });
    }
  };

  std::shared_ptr<Sca> mSca;
};

struct ScaOperationDataVectorToPythonListOfTuples {
  static PyObject* convert(const std::vector<std::pair<Sca::Operation, Sca::Data>>& seqOut)
  {
    bp::list ret;
    for (const auto& el : seqOut) {
      ret.append(bp::make_tuple(Sca::ScaOperationToString(el.first), el.second));
    }
    return bp::incref(bp::object(ret).ptr());
  }
};

struct ScaDataToPython {
  struct ScaDataConvertVisitor : public boost::static_visitor<PyObject*> {
    auto operator()(Sca::CommandData commandData) const
    {
      bp::tuple ret = bp::make_tuple(commandData.command, commandData.data);
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

  static PyObject* convert(Sca::Data data)
  {
    return boost::apply_visitor(ScaDataConvertVisitor(), data);
  }
};

} //namespace python
} //namespace alf
} //namespace o2

#endif // O2_ALF_SRC_SCAPYTHONINTERFACE_H
