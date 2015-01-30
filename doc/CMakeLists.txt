foreach (program doxygen sphinx-build)
  find_program(${program} ${program})
  if (NOT ${program})
    message(STATUS "Target 'doc' disabled (requires ${program})")
    return ()
  endif ()
endforeach ()

add_custom_target(doc
  COMMAND ${doxygen}
  COMMAND ${sphinx-build} -b html . html)
