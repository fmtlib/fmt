find_program(DOXYGEN doxygen)
if (NOT DOXYGEN)
  message(STATUS "Target 'doc' disabled (requires doxygen)")
  return ()
endif ()

find_package(PythonInterp QUIET REQUIRED)
find_program(SPHINX_EXECUTABLE NAMES sphinx-build sphinx-build-3)

add_custom_target(doc
  COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/build.py
                               ${SPHINX_EXECUTABLE} ${FMT_VERSION}
  SOURCES api.rst syntax.rst usage.rst build.py conf.py _templates/layout.html)

include(GNUInstallDirs)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html/
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/doc/fmt OPTIONAL
        PATTERN ".doctrees" EXCLUDE)
