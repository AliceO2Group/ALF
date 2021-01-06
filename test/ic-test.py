#!/usr/bin/env python

import unittest
import libO2Alf
import time

class TestIc(unittest.TestCase):
  def test_init(self):
    Ic = libO2Alf.IcInterface("#1", 0)
  
  def test_init_fails(self):
    with self.assertRaises(RuntimeError):
      ic = libO2Alf.IcInterface("#1", 42) # linkId out of range

  def test_sc_reset(self):
    ic = libO2Alf.IcInterface("#1", 0)
    ic.sc_reset()

  def test_set_channel(self):
    ic = libO2Alf.IcInterface("#1", 0)
    ic.set_channel(0)

  def test_set_channel_fails(self):
    ic = libO2Alf.IcInterface("#1", 0)
    with self.assertRaises(RuntimeError):
      ic.set_channel(42) #linkId out of range

#  def test_write_read(self): #TODO: uncomment
#    ic = libO2Alf.IcInterface("#1", 0)
#    ic.sc_reset();
#    ic.write(0xbb, 0xdd)
#    self.assertEqual(ic.read(0xbb), 0xdd)

  def test_write_reset_read(self):
    ic = libO2Alf.IcInterface("#1", 0)
    ic.write(0xbb, 0xdd)
    ic.sc_reset();
    self.assertNotEqual(ic.read(0xbb), 0xdd)

  def test_sequence_zero(self): #TODO: Add a non-failing
      ic = libO2Alf.IcInterface("#1", 0)
      seq = [("write", (0xbb, 0xdd)), 
             ("read", 0xbb)];
      out = ic.sequence(seq)
      self.assertEqual(out, 
                       [("write", (0xbb, 0xdd)),
                        ("read", 0x0)])
