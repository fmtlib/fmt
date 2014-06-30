/*
 Custom Google Test assertions.

 Copyright (c) 2012-2014, Victor Zverovich
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "gtest-extra.h"

#if FMT_USE_FILE_DESCRIPTORS

using fmt::File;

void OutputRedirect::Flush() {
#if EOF != -1
# error "FMT_RETRY assumes return value of -1 indicating failure"
#endif
  int result = 0;
  FMT_RETRY(result, fflush(file_));
  if (result != 0)
    throw fmt::SystemError(errno, "cannot flush stream");
}

void OutputRedirect::Restore() {
  if (original_.descriptor() == -1)
    return;  // Already restored.
  Flush();
  // Restore the original file.
  original_.dup2(FMT_POSIX(fileno(file_)));
  original_.close();
}

OutputRedirect::OutputRedirect(FILE *file) : file_(file) {
  Flush();
  int fd = FMT_POSIX(fileno(file));
  // Create a File object referring to the original file.
  original_ = File::dup(fd);
  // Create a pipe.
  File write_end;
  File::pipe(read_end_, write_end);
  // Connect the passed FILE object to the write end of the pipe.
  write_end.dup2(fd);
}

OutputRedirect::~OutputRedirect() FMT_NOEXCEPT(true) {
  try {
    Restore();
  } catch (const std::exception &e) {
    std::fputs(e.what(), stderr);
  }
}

std::string OutputRedirect::RestoreAndRead() {
  // Restore output.
  Restore();

  // Read everything from the pipe.
  std::string content;
  if (read_end_.descriptor() == -1)
    return content;  // Already read.
  enum { BUFFER_SIZE = 4096 };
  char buffer[BUFFER_SIZE];
  std::streamsize count = 0;
  do {
    count = read_end_.read(buffer, BUFFER_SIZE);
    content.append(buffer, static_cast<std::size_t>(count));
  } while (count != 0);
  read_end_.close();
  return content;
}

#endif  // FMT_USE_FILE_DESCRIPTORS

std::string FormatSystemErrorMessage(int error_code, fmt::StringRef message) {
  fmt::Writer out;
  fmt::internal::FormatSystemErrorMessage(out, error_code, message);
  return out.str();
}
