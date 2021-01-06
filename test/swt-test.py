#!/usr/bin/env python

import unittest
import libO2Alf
import time
import threading

class TestSwt(unittest.TestCase):
  def test_init(self):
    swt = libO2Alf.SwtInterface("#1", 0)
  
  def test_init_fails(self):
    with self.assertRaises(RuntimeError):
      swt = libO2Alf.SwtInterface("#1", 42) # linkId out of range

  def test_sc_reset(self):
    swt = libO2Alf.SwtInterface("#1", 0)
    swt.sc_reset()

  def test_set_channel(self):
    swt = libO2Alf.SwtInterface("#1", 0)
    swt.set_channel(0)

  def test_set_channel_fails(self):
    swt = libO2Alf.SwtInterface("#1", 0)
    with self.assertRaises(RuntimeError):
      swt.set_channel(42) #linkId out of range

  def test_write_read(self):
    swt = libO2Alf.SwtInterface("#1", 0)
    swt.sc_reset();
    swt.write(0xdd)
    words = swt.read()
    self.assertEqual(words, [(0xdd)])

  def test_write_reset_read(self):
    swt = libO2Alf.SwtInterface("#1", 0)
    swt.write(0xdd)
    swt.sc_reset();
    with self.assertRaises(RuntimeError):
      words = swt.read()

  def test_write_read_time(self):
    swt = libO2Alf.SwtInterface("#1", 0)
    swt.sc_reset();
    swt.write(0xdd)
    words = swt.read(20)
    self.assertEqual(words, [(0xdd)])
 
  def test_write_read_time_fails(self):
    swt = libO2Alf.SwtInterface("#1", 0)
    swt.write(0xdd)
    swt.sc_reset()
    start = time.time()
    with self.assertRaises(RuntimeError):
      words = swt.read(100)
    end = time.time()
    self.assertGreater(end-start, 0.1)

  def test_sequence(self):
      swt = libO2Alf.SwtInterface("#1", 0)
      swt.sc_reset()
      sequence = [("write", 0xdeadbeef), 
                  ("write", 0xcafecafe),
                  ("read")];
      out = swt.sequence(sequence)
      self.assertEqual(out, 
                       [("write", 0xdeadbeef),
                        ("write", 0xcafecafe),
                        ("read", 0xdeadbeef),
                        ("read", 0xcafecafe)])

  def test_sequence_wait(self):
        swt = libO2Alf.SwtInterface("#1", 0)
        swt.sc_reset()
        sequence = [("write", 0xdeadbeef), 
                    ("wait", 100),
                    ("read", 80)];
        start = time.time()
        out = swt.sequence(sequence)
        end = time.time()
        self.assertEqual(out, 
                         [("write", 0xdeadbeef),
                          ("wait", 100),
                          ("read", 0xdeadbeef)])
        self.assertGreater(end - start, 0.1)

  def test_sequence_reset(self):
    swt = libO2Alf.SwtInterface("#1", 0)
    sequence = [("write", 0xdeadbeef), 
                ("sc_reset"),
                ("write", 0xcafecafe),
                ("read", 80)];
    out = swt.sequence(sequence)
    self.assertEqual(out, 
                     [("write", 0xdeadbeef),
                      ("sc_reset", ""),
                      ("write", 0xcafecafe),
                      ("read", 0xcafecafe)])

  def test_sequence_error(self):
    swt = libO2Alf.SwtInterface("#1", 0)
    sequence = [("write", 0xdeadbeef), 
                ("sc_reset"),
                ("read", 80)];
    out = swt.sequence(sequence)
    self.assertEqual(out[2][0], "error")
  
  def test_sequence_lock(self):
        swt = libO2Alf.SwtInterface("#1", 0)
        swt.sc_reset()
        swt_w = libO2Alf.SwtInterface("#1", 1)
        swt_w.sc_reset()

        sequence = [("write", 0xdeadbeef), 
                    ("wait", 200),
                    ("read", 80)];
        t = threading.Thread(target=swt.sequence, args=(sequence, True))
        t.start()
        with self.assertRaises(RuntimeError):
          time.sleep(0.05)
          swt_w.sequence(sequence, True)
        t.join()
