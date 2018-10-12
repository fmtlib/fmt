find_program(DOXYGEN doxygen)
if (NOT DOXYGEN)
  message(STATUS "Target 'doc' disabled (requires doxygen)")
  return ()
endif ()

find_package(PythonInterp
  QUIET REQUIRED)
find_program(SPHINX_EXECUTABLE
  NAMES sphinx-build sphinx-build-3)
add_custom_target(doc
  COMMAND ${CMAKE_COMMAND} -E env SPHINX_EXECUTABLE=${SPHINX_EXECUTABLE}
    ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/build.py ${FMT_VERSION}
  SOURCES api.rst syntax.rst build.py conf.py _templates/layout.html)

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html/
        DESTINATION share/doc/fmt OPTIONAL)
