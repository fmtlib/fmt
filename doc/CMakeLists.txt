find_program(DOXYGEN doxygen
  PATHS "$ENV{ProgramFiles}/doxygen/bin"
        "$ENV{ProgramFiles\(x86\)}/doxygen/bin")
if (NOT DOXYGEN)
  message(STATUS "Target 'doc' disabled (requires doxygen)")
  return ()
endif ()

find_package(PythonInterp QUIET REQUIRED)

add_custom_target(doc
  COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/build.py
                               ${FMT_VERSION}
  SOURCES api.rst syntax.rst usage.rst build.py conf.py _templates/layout.html)

include(GNUInstallDirs)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html/
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/doc/fmt OPTIONAL
        PATTERN ".doctrees" EXCLUDE)
