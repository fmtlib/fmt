#------------------------------------------------------------------------------
# define the cppformat library, its includes and the needed defines
set(FMT_SOURCES format.cc format.h)
if (HAVE_OPEN)
  set(FMT_SOURCES ${FMT_SOURCES} posix.cc posix.h)
endif ()

add_library(cppformat ${FMT_SOURCES})

target_compile_options(cppformat PUBLIC ${CPP11_FLAG}) # starting with cmake 3.1 the CXX_STANDARD property can be used
if (FMT_PEDANTIC)
  target_compile_options(cppformat PRIVATE ${PEDANTIC_COMPILE_FLAGS})
endif ()

target_compile_definitions(cppformat INTERFACE
  FMT_USE_FILE_DESCRIPTORS=$<BOOL:${HAVE_OPEN}>)

target_include_directories(cppformat INTERFACE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>)

set_target_properties(cppformat PROPERTIES
  VERSION ${CPPFORMAT_VERSION} SOVERSION ${CPACK_PACKAGE_VERSION_MAJOR})

if (BUILD_SHARED_LIBS)
  if (UNIX AND NOT APPLE)
    # Fix rpmlint warning:
    # unused-direct-shlib-dependency /usr/lib/libformat.so.1.1.0 /lib/libm.so.6.
    target_link_libraries(cppformat -Wl,--as-needed)
  endif ()
  target_compile_definitions(cppformat PRIVATE FMT_EXPORT INTERFACE FMT_SHARED)
endif ()

#------------------------------------------------------------------------------
# additionally define a header only library when cmake is new enough
if (CMAKE_VERSION VERSION_GREATER 3.1.0 OR CMAKE_VERSION VERSION_EQUAL 3.1.0)
  add_library(cppformat-header-only INTERFACE)

  target_compile_definitions(cppformat-header-only INTERFACE FMT_HEADER_ONLY=1)

  target_include_directories(cppformat-header-only INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>)
endif ()

# Install targets.
if (FMT_INSTALL)
  include(CMakePackageConfigHelpers)
  set(config_install_dir lib/cmake/cppformat)
  set(version_config ${PROJECT_BINARY_DIR}/cppformat-config-version.cmake)
  set(project_config ${PROJECT_BINARY_DIR}/cppformat-config.cmake)
  set(targets_export_name cppformat-targets)

  set(FMT_LIB_DIR lib CACHE STRING
    "Installation directory for libraries, relative to ${CMAKE_INSTALL_PREFIX}.")

  # Generate the version, config and target files into the build directory.
  write_basic_package_version_file(
    ${version_config}
    VERSION ${CPPFORMAT_VERSION}
    COMPATIBILITY AnyNewerVersion)
  configure_package_config_file(
    ${PROJECT_SOURCE_DIR}/support/cmake/cppformat-config.cmake.in
    ${project_config}
    INSTALL_DESTINATION ${config_install_dir})

  if (TARGET cppformat-header-only)
    export(TARGETS cppformat cppformat-header-only FILE ${PROJECT_BINARY_DIR}/${targets_export_name}.cmake)
  else ()
    export(TARGETS cppformat FILE ${PROJECT_BINARY_DIR}/${targets_export_name}.cmake)
  endif ()

  # Install version, config and target files.
  install(
    FILES ${project_config} ${version_config}
    DESTINATION ${config_install_dir})
  install(EXPORT ${targets_export_name} DESTINATION ${config_install_dir})

  # Install the library and the include file.
  install(TARGETS cppformat EXPORT ${targets_export_name} DESTINATION ${FMT_LIB_DIR})
  install(FILES format.h DESTINATION include/cppformat)
endif ()
