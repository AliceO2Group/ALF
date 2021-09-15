
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

/// \file SwtPythonInterface.cxx
/// \brief Python SWT interface for the Slow Control library
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef O2_ALF_SRC_SWTPYTHONINTERFACE_H
#define O2_ALF_SRC_SWTPYTHONINTERFACE_H

#include <boost/python.hpp>
#include "Alf/Swt.h"
#include "PythonInterface.h"

namespace o2
{
namespace alf
{
namespace python
{

namespace bp = boost::python;
typedef boost::variant<std::string, std::pair<std::string, uint32_t>> SwtArgsVariant;

/// Exposed python function docs
auto sSwtInitDoc =
  R"(Initializes an Swt object

Args:
    card id: String containing PCI address (e.g. 42:0.0), serial-endpoint pair (e.g. 10241:1), sequence number (e.g. #2)
    channel: Channel number to initially set)";

auto sSwtSetChannelDoc =
  R"(Sets the Slow Control channel

Args:
    channel: Channel number to set)";

auto sSwtScResetDoc =
  R"(Resets Slow Control)";

auto sSwtWriteDoc =
  R"(Perform an SWT write
  
Args:
    data: SWT word (unsigned 32-bit) to write)";

auto sSwtReadDoc =
  R"(Perform an SWT read

Args:
  timeout(optional): Timeout in ms (int)

Returns:
  A list of SWT words(unsigned 32-bit) read out)";

auto sSwtSequenceDoc =
  R"(Execute an SWT sequence:

Args:
  sequence: A list of tuples made up of:
    operation: An operation(string) to perform
    input: Input(unsigned 32-bit int, int) to said operation

    Operations and input types:
      operation: "sc_reset"
      input: no-input

      operation: "wait"
      input: wait_time (optional int)

      operation: "write"
      input: SWT word (unsigned 32-bit int)

      operation: "read"
      input: time_out (optional int)

  lock: boolean to execute the sequence within an LLA session

Returns:
  sequence: A list of tuples made up of: 
    operation: The operation carried out (string, same as input)
    output: Output (string, unsigned 32-bit int, int) of said operation)
  
      Output per operation:
        operation: "sc_reset"
        output: "" (string)

        operation: "wait"
        output: wait_time (int)

        operation: "write"
        output: echo of input (unsigned 32-bit int)

        operation: "read"
        output: a "read" operation tuple for each word read out (unsigned 32-bit int)
)";

struct SwtArgsVariantConverter {
  SwtArgsVariantConverter&
    fromPython()
  {
    bp::converter::registry::push_back(
      &SwtArgsVariantConverter::convertible,
      &SwtArgsVariantConverter::construct,
      bp::type_id<SwtArgsVariant>());

    return *this;
  }

  static void* convertible(PyObject* obj)
  {
    return obj;
  }

  static void construct(PyObject* obj, bp::converter::rvalue_from_python_stage1_data* data)
  {
    bp::handle<> handle(bp::borrowed(obj));
    void* storage = ((bp::converter::rvalue_from_python_storage<SwtArgsVariant>*)data)->storage.bytes;

    SwtArgsVariant ret;

    bp::extract<std::string> s(obj);
    if (s.check()) {
      ret = s();
    } else {
      bp::tuple tuple(bp::borrowed(obj));
      ret = std::pair<std::string, uint32_t>(bp::extract<std::string>(tuple[0]), bp::extract<uint32_t>(tuple[1]));
    }

    new (storage) SwtArgsVariant(ret);
    data->convertible = storage;
    return;
  }
};

class SwtInterface
{
 public:
  SwtInterface(std::string cardIdString, int linkId = -1)
  {
    mSwt = std::make_shared<Swt>(cardIdString, linkId);
  }

  void setChannel(int gbtChannel)
  {
    mSwt->setChannel(gbtChannel);
  }

  void scReset()
  {
    mSwt->scReset();
  }

  void write(uint32_t low)
  {
    SwtWord swtWord = SwtWord(low, 0x0, 0x0);
    mSwt->write(swtWord);
  }

  std::vector<SwtWord> readDefault(Swt::TimeOut msTimeOut = Swt::DEFAULT_SWT_TIMEOUT_MS)
  {
    return mSwt->read(SwtWord::Size::Low, msTimeOut);
  }

  std::vector<SwtWord> read()
  {
    return readDefault();
  }

  std::vector<std::pair<Swt::Operation, Swt::Data>> sequence(std::vector<SwtArgsVariant> sequence, bool lock = false)
  {
    ScopedGILRelease s; // enable boost::python multi-threading
    std::vector<std::pair<Swt::Operation, Swt::Data>> swtSequence;
    for (const auto& v : sequence) {
      swtSequence.push_back(boost::apply_visitor(SwtArgsVariantVisitor(), v));
    }

    auto out = mSwt->executeSequence(swtSequence, lock);
    return out;
  }

  std::vector<std::pair<Swt::Operation, Swt::Data>> sequenceDefault(std::vector<SwtArgsVariant> swtSeq)
  {
    return sequence(swtSeq);
  }

 private:
  struct SwtArgsVariantVisitor
    : public boost::static_visitor<std::pair<Swt::Operation, Swt::Data>> {
    auto operator()(std::string v) const
    {
      auto op = Swt::StringToSwtOperation(v);
      return std::make_pair(op, op == Swt::Operation::Read ? Swt::DEFAULT_SWT_TIMEOUT_MS : 0);
    }

    auto operator()(std::pair<std::string, uint32_t> v) const
    {
      auto op = Swt::StringToSwtOperation(v.first);
      Swt::Data swtData;
      if (op == Swt::Operation::Write) {
        swtData = SwtWord{ boost::get<uint32_t>(v.second) };
      } else {
        swtData = boost::get<int>(v.second);
      }

      return std::make_pair(op, swtData);
    }
  };

  std::shared_ptr<Swt> mSwt;
};

struct SwtWordVectorToPythonList {
  static PyObject* convert(const std::vector<SwtWord>& words)
  {
    bp::list ret;
    for (const auto& v : words) {
      ret.append(v.getLow());
    }
    return bp::incref(bp::object(ret).ptr());
  }
};

struct SwtOperationDataVectorToPythonListOfTuples {
  static PyObject* convert(const std::vector<std::pair<Swt::Operation, Swt::Data>>& seqOut)
  {
    bp::list ret;
    for (const auto& el : seqOut) {
      ret.append(bp::make_tuple(Swt::SwtOperationToString(el.first), el.second));
    }
    return bp::incref(bp::object(ret).ptr());
  }
};

struct SwtDataToPython {
  struct SwtDataConvertVisitor : public boost::static_visitor<PyObject*> {
    auto operator()(SwtWord data) const
    {
      return bp::incref(bp::object(data.getLow()).ptr());
    }

    auto operator()(std::string data) const
    {
      return bp::incref(bp::object(data).ptr());
    }

    auto operator()(int data) const
    {
      return bp::incref(bp::object(data).ptr());
    }

    auto operator()(boost::blank) const
    {
      std::string data = "";
      return bp::xincref(bp::object(data).ptr());
    }
  };

  static PyObject* convert(Swt::Data data)
  {
    return boost::apply_visitor(SwtDataConvertVisitor(), data);
  }
};

} //namespace python
} //namespace alf
} //namespace o2

#endif // O2_ALF_SRC_SWTPYTHONINTERFACE_H
