#!/usr/bin/env python

import unittest
import libO2Alf
import time

class TestSca(unittest.TestCase):
  def test_init(self):
    sca = libO2Alf.ScaInterface("#1", 0)
  
  def test_init_fails(self):
    with self.assertRaises(RuntimeError):
      sca = libO2Alf.ScaInterface("#1", 42) # linkId out of rangee

  def test_sc_reset(self):
        sca = libO2Alf.ScaInterface("#1", 0)
        sca.sc_reset()

  def test_set_channel(self):
    sca = libO2Alf.ScaInterface("#1", 0)
    sca.set_channel(0)

  def test_set_channel_fails(self):
    sca = libO2Alf.ScaInterface("#1", 0)
    with self.assertRaises(RuntimeError):
      sca.set_channel(42) #linkId out of range

  def test_svl_reset(self): #placeholder
    sca = libO2Alf.ScaInterface("#1", 0)
    sca.svl_reset()

  def test_svl_connect(self): #placeholder
    sca = libO2Alf.ScaInterface("#1", 0)
    sca.svl_connect()

  def test_execute_command_fails(self): #TODO: w/ expected output
    sca = libO2Alf.ScaInterface("#1", 0)
    with self.assertRaises(RuntimeError):
      sca.execute_command(0xcafe, 0x1234)

  def test_sequence_error(self): #TODO: update
    sca = libO2Alf.ScaInterface("#1", 0)
    seq = [("command", (0xdeadbeef,0xfec)),
          ("command", (42, 54))]
    out = sca.sequence(seq)
    self.assertEqual(out[0][0], "error")
