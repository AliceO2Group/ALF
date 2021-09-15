
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

/// \file PythonInterface.cxx
/// \brief Python interface for the Slow Control library
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <boost/python.hpp>

#include "IcPythonInterface.h"
#include "ScaPythonInterface.h"
#include "SwtPythonInterface.h"

namespace o2
{
namespace alf
{
namespace python
{

namespace bp = boost::python;

struct pairConverter {
  template <typename T1, typename T2>
  pairConverter&
    fromPython()
  {
    bp::converter::registry::push_back(
      &pairConverter::convertible,
      &pairConverter::construct<T1, T2>,
      bp::type_id<std::pair<T1, T2>>());

    return *this;
  }

  static void* convertible(PyObject* obj)
  {
    if (!PyTuple_CheckExact(obj))
      return 0;
    if (PyTuple_Size(obj) != 2)
      return 0;
    return obj;
  }

  template <typename T1, typename T2>
  static void construct(PyObject* obj, bp::converter::rvalue_from_python_stage1_data* data)
  {
    bp::tuple tuple(bp::borrowed(obj));
    void* storage = ((bp::converter::rvalue_from_python_storage<std::pair<T1, T2>>*)data)->storage.bytes;
    new (storage) std::pair<T1, T2>(bp::extract<T1>(tuple[0]), bp::extract<T2>(tuple[1]));
    data->convertible = storage;
  }
};

struct iterableConverter {
  template <typename T>
  iterableConverter&
    fromPython()
  {
    bp::converter::registry::push_back(
      &iterableConverter::convertible,
      &iterableConverter::construct<T>,
      bp::type_id<T>());

    return *this;
  }

  static void* convertible(PyObject* object)
  {
    return PyObject_GetIter(object) ? object : NULL;
  }

  template <typename T>
  static void construct(
    PyObject* object,
    bp::converter::rvalue_from_python_stage1_data* data)
  {
    bp::handle<> handle(bp::borrowed(object));

    typedef bp::converter::rvalue_from_python_storage<T> storage_type;
    void* storage = reinterpret_cast<storage_type*>(data)->storage.bytes;

    typedef bp::stl_input_iterator<typename T::value_type> iterator;

    new (storage) T(
      iterator(bp::object(handle)),
      iterator());
    data->convertible = storage;
  }
};

BOOST_PYTHON_MODULE(libO2Alf)
{
  //PyEval_InitThreads(); // enable boost::python multi-threading

  bp::to_python_converter<std::vector<std::pair<Ic::Operation, Ic::Data>>, IcOperationDataVectorToPythonListOfTuples>();
  bp::to_python_converter<Ic::Data, IcDataToPython>();

  bp::to_python_converter<std::vector<std::pair<Sca::Operation, Sca::Data>>, ScaOperationDataVectorToPythonListOfTuples>();
  bp::to_python_converter<Sca::Data, ScaDataToPython>();

  bp::to_python_converter<std::vector<SwtWord>, SwtWordVectorToPythonList>();
  bp::to_python_converter<std::vector<std::pair<Swt::Operation, Swt::Data>>, SwtOperationDataVectorToPythonListOfTuples>();
  bp::to_python_converter<Swt::Data, SwtDataToPython>();

  pairConverter()
    .fromPython<uint32_t, uint32_t>()
    .fromPython<std::string, uint32_t>();

  IcArgsVariantConverter()
    .fromPython();

  ScaArgsVariantConverter()
    .fromPython();

  SwtArgsVariantConverter()
    .fromPython();

  iterableConverter()
    .fromPython<std::vector<IcArgsVariant>>()
    .fromPython<std::vector<ScaArgsVariant>>()
    .fromPython<std::vector<SwtArgsVariant>>();

  bp::class_<SwtInterface>("SwtInterface", bp::init<std::string, int>(sSwtInitDoc))
    .def("set_channel", &SwtInterface::setChannel, sSwtSetChannelDoc)
    .def("sc_reset", &SwtInterface::scReset, sSwtScResetDoc)
    .def("write", &SwtInterface::write, sSwtWriteDoc)
    .def("read", &SwtInterface::read, sSwtReadDoc)
    .def("read", &SwtInterface::readDefault, sSwtReadDoc)
    .def("sequence", &SwtInterface::sequence, sSwtSequenceDoc)
    .def("sequence", &SwtInterface::sequenceDefault, sSwtSequenceDoc);

  bp::class_<ScaInterface>("ScaInterface", bp::init<std::string, int>(sScaInitDoc))
    .def("set_channel", &ScaInterface::setChannel, sScaSetChannelDoc)
    .def("sc_reset", &ScaInterface::scReset, sScaScResetDoc)
    .def("svl_reset", &ScaInterface::svlReset, sScaSvlResetDoc)
    .def("svl_connect", &ScaInterface::svlConnect, sScaSvlConnectDoc)
    .def("execute_command", &ScaInterface::executeCommand, sScaExecuteCommandDoc)
    .def("sequence", &ScaInterface::sequence, sScaSequenceDoc)
    .def("sequence", &ScaInterface::sequenceDefault, sScaSequenceDoc);

  bp::class_<IcInterface>("IcInterface", bp::init<std::string, int>(sIcInitDoc))
    .def("set_channel", &IcInterface::setChannel, sIcSetChannelDoc)
    .def("sc_reset", &IcInterface::scReset, sIcScResetDoc)
    .def("read", &IcInterface::read, sIcReadDoc)
    .def("write", &IcInterface::write, sIcWriteDoc)
    .def("write_gbt_i2c", &IcInterface::writeGbtI2c, sIcWriteGbtI2cDoc)
    .def("sequence", &IcInterface::sequence, sIcSequenceDoc)
    .def("sequence", &IcInterface::sequenceDefault, sIcSequenceDoc);
}

} // namespace python
} // namespace alf
} // namespace o2
