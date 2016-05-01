*****
Usage
*****

To use the fmt library, add :file:`format.h` and :file:`format.cc` from
a `release archive <https://github.com/fmtlib/fmt/releases/latest>`_
or the `Git repository <https://github.com/fmtlib/fmt>`_ to your project.
Alternatively, you can :ref:`build the library with CMake <building>`.

If you are using Visual C++ with precompiled headers, you might need to add
the line ::

   #include "stdafx.h"

before other includes in :file:`format.cc`.

.. _building:

Building the library
====================

The included `CMake build script`__ can be used to build the fmt
library on a wide range of platforms. CMake is freely available for
download from http://www.cmake.org/download/.

__ https://github.com/fmtlib/fmt/blob/master/CMakeLists.txt

CMake works by generating native makefiles or project files that can
be used in the compiler environment of your choice. The typical
workflow starts with::

  mkdir build          # Create a directory to hold the build output.
  cd build
  cmake <path/to/fmt>  # Generate native build scripts.

where :file:`{<path/to/fmt>}` is a path to the ``fmt`` repository.

If you are on a \*nix system, you should now see a Makefile in the
current directory. Now you can build the library by running :command:`make`.

Once the library has been built you can invoke :command:`make test` to run
the tests.

If you use Windows and have Visual Studio installed, a :file:`FORMAT.sln`
file and several :file:`.vcproj` files will be created. You can then build them
using Visual Studio or msbuild.

On Mac OS X with Xcode installed, an :file:`.xcodeproj` file will be generated.

To build a `shared library`__ set the ``BUILD_SHARED_LIBS`` CMake variable to
``TRUE``::

  cmake -DBUILD_SHARED_LIBS=TRUE ...

__ http://en.wikipedia.org/wiki/Library_%28computing%29#Shared_libraries

Building the documentation
==========================

To build the documentation you need the following software installed on your
system:

* `Python <https://www.python.org/>`_ with pip and virtualenv
* `Doxygen <http://www.stack.nl/~dimitri/doxygen/>`_
* `Less <http://lesscss.org/>`_ with less-plugin-clean-css

First generate makefiles or project files using CMake as described in
the previous section. Then compile the ``doc`` target/project, for example::

  make doc

This will generate the HTML documentation in ``doc/html``.
  
Android NDK
===========

fmt provides `Android.mk file`__ that can be used to build the library
with `Android NDK <https://developer.android.com/tools/sdk/ndk/index.html>`_.
For an example of using fmt with Android NDK, see the
`android-ndk-example <https://github.com/fmtlib/android-ndk-example>`_
repository.

__ https://github.com/fmtlib/fmt/blob/master/Android.mk

Homebrew
========

fmt can be installed on OS X using `Homebrew <http://brew.sh/>`_::

  brew install cppformat
