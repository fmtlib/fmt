# A file downloader.

import contextlib, os, tempfile, timer, urllib2, urlparse

class Downloader:
  def __init__(self, dir=None):
    self.dir = dir

  # Downloads a file and removes it when exiting a block.
  # Usage:
  #  d = Downloader()
  #  with d.download(url) as f:
  #    use_file(f)
  def download(self, url, cookie=None):
    suffix = os.path.splitext(urlparse.urlsplit(url)[2])[1]
    fd, filename = tempfile.mkstemp(suffix=suffix, dir=self.dir)
    os.close(fd)
    with timer.print_time('Downloading', url, 'to', filename):
      opener = urllib2.build_opener()
      if cookie:
        opener.addheaders.append(('Cookie', cookie))
      num_tries = 2
      for i in range(num_tries):
        try:
          f = opener.open(url)
        except urllib2.URLError, e:
          print('Failed to open url', url)
          continue
        length = f.headers.get('content-length')
        if not length:
          print('Failed to get content-length')
          continue
        length = int(length)
        with open(filename, 'wb') as out:
          count = 0
          while count < length:
            data = f.read(1024 * 1024)
            count += len(data)
            out.write(data)
    @contextlib.contextmanager
    def remove(filename):
      try:
        yield filename
      finally:
        os.remove(filename)
    return remove(filename)
