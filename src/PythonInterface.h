
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

/// \file PythonInterface.h
/// \brief Python interface for the Slow Control library
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch))

#ifndef O2_ALF_SRC_PYTHONINTERFACE_H
#define O2_ALF_SRC_PYTHONINTERFACE_H

#include <boost/python.hpp>

namespace o2
{
namespace alf
{
namespace python
{

class ScopedGILRelease
{
 public:
  ScopedGILRelease()
  {
    mThreadState = PyEval_SaveThread();
  }

  ~ScopedGILRelease()
  {
    PyEval_RestoreThread(mThreadState);
    mThreadState = NULL;
  }

 private:
  PyThreadState* mThreadState;
};

} // namespace python
} // namespace alf
} // namespace o2

#endif // O2_ALF_SRC_PYTHONINTERFACE_H
