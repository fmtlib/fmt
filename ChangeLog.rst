0.9.0 - 2014-05-13
------------------

* More efficient implementation of variadic formatting functions.

* ``Writer::Format`` now has a variadic overload:

  .. code:: c++

    Writer out;
    out.Format("Look, I'm {}!", "variadic");

* For efficiency and consistency with other overloads, variadic overload of
  the ``Format`` function now returns ``Writer`` instead of ``std::string``.
  Use the ``str`` function to convert it to ``std::string``:

  .. code:: c++

    std::string s = str(Format("Look, I'm {}!", "variadic"));

* Replaced formatter actions with output sinks: ``NoAction`` -> ``NullSink``,
  ``Write`` -> ``FileSink``, ``ColorWriter`` -> ``ANSITerminalSink``.
  This improves naming consistency and shouldn't affect client code unless
  these classes are used directly which should be rarely needed.

* Added ``ThrowSystemError`` function that formats a message and throws
  ``SystemError`` containing the formatted message and system-specific error
  description. For example, the following code

  .. code:: c++

    FILE *f = fopen(filename, "r");
    if (!f)
      ThrowSystemError(errno, "Failed to open file '{}'") << filename;

  will throw ``SystemError`` exception with description
  "Failed to open file '<filename>': No such file or directory" if file
  doesn't exist.

* Support for `AppVeyor <https://ci.appveyor.com/project/vitaut/cppformat>`_
  continuous integration platform.

* ``Format`` now throws ``SystemError`` in case of I/O errors.

* Improve test infrastructure. Print functions are now tested by redirecting
  the output to a pipe.

0.8.0 - 2014-04-14
------------------

* Initial release
