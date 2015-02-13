# A with statement based timer.

from __future__ import print_function
from contextlib import contextmanager
import timeit

class Timer:
  """
  A with statement based timer.
  Usage:
    t = Timer()
    with t:
      do_something()
    time = t.time
  """
    
  def __enter__(self):
    self.start = timeit.default_timer()

  def __exit__(self, type, value, traceback):
    finish = timeit.default_timer()
    self.time = finish - self.start

@contextmanager
def print_time(*args):
  """
  Measures and prints the time taken to execute nested code.
  args: Additional arguments to print.
  """
  t = Timer()
  print(*args)
  with t:
    yield
  print(*args, end=' ')
  print('finished in {0:.2f} second(s)'.format(t.time))
