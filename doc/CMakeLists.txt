find_program(DOXYGEN doxygen)
if (NOT DOXYGEN)
  message(STATUS "Target 'doc' disabled (requires doxygen)")
  return ()
endif ()

add_custom_command(OUTPUT html/_static/cppformat.css
  COMMAND lessc --clean-css --include-path=bootstrap cppformat.less html/_static/cppformat.css
  DEPENDS cppformat.less)

add_custom_target(doc COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/build.py
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/html/_static/cppformat.css)

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html/ DESTINATION share/doc/cppformat)
