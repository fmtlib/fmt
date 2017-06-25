# Define the fmt library, its includes and the needed defines.
# *.cc are added to FMT_HEADERS for the header-only configuration.
set(FMT_HEADERS container.h format.h format.cc ostream.h ostream.cc printf.h
                printf.cc string.h time.h)
if (HAVE_OPEN)
  set(FMT_HEADERS ${FMT_HEADERS} posix.h)
  set(FMT_SOURCES ${FMT_SOURCES} posix.cc)
endif ()

add_library(fmt ${FMT_SOURCES} ${FMT_HEADERS} ../README.rst ../ChangeLog.rst)
add_library(fmt::fmt ALIAS fmt)

# Starting with cmake 3.1 the CXX_STANDARD property can be used instead.
# Note: Don't make -std=c++11 public or interface, since it breaks projects
# that use C++14.
target_compile_options(fmt PRIVATE ${CPP11_FLAG})
if (FMT_PEDANTIC)
  target_compile_options(fmt PRIVATE ${PEDANTIC_COMPILE_FLAGS})
endif ()

target_include_directories(fmt PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>)

set_target_properties(fmt PROPERTIES
  VERSION ${FMT_VERSION} SOVERSION ${CPACK_PACKAGE_VERSION_MAJOR})

if (BUILD_SHARED_LIBS)
  if (UNIX AND NOT APPLE)
    # Fix rpmlint warning:
    # unused-direct-shlib-dependency /usr/lib/libformat.so.1.1.0 /lib/libm.so.6.
    target_link_libraries(fmt -Wl,--as-needed)
  endif ()
  target_compile_definitions(fmt PRIVATE FMT_EXPORT INTERFACE FMT_SHARED)
endif ()

#------------------------------------------------------------------------------
# additionally define a header only library when cmake is new enough
if (CMAKE_VERSION VERSION_GREATER 3.1.0 OR CMAKE_VERSION VERSION_EQUAL 3.1.0)
  add_library(fmt-header-only INTERFACE)
  add_library(fmt::fmt-header-only ALIAS fmt-header-only)

  target_compile_definitions(fmt-header-only INTERFACE FMT_HEADER_ONLY=1)

  target_include_directories(fmt-header-only INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>)
endif ()

# Install targets.
if (FMT_INSTALL)
  include(CMakePackageConfigHelpers)
  set(FMT_CMAKE_DIR lib/cmake/fmt CACHE STRING
    "Installation directory for cmake files, relative to ${CMAKE_INSTALL_PREFIX}.")
  set(version_config ${PROJECT_BINARY_DIR}/fmt-config-version.cmake)
  set(project_config ${PROJECT_BINARY_DIR}/fmt-config.cmake)
  set(targets_export_name fmt-targets)

  set (INSTALL_TARGETS fmt)
  if (TARGET fmt-header-only)
    set(INSTALL_TARGETS ${INSTALL_TARGETS} fmt-header-only)
  endif ()

  set(FMT_LIB_DIR lib CACHE STRING
    "Installation directory for libraries, relative to ${CMAKE_INSTALL_PREFIX}.")

  # Generate the version, config and target files into the build directory.
  write_basic_package_version_file(
    ${version_config}
    VERSION ${FMT_VERSION}
    COMPATIBILITY AnyNewerVersion)
  configure_package_config_file(
    ${PROJECT_SOURCE_DIR}/support/cmake/fmt-config.cmake.in
    ${project_config}
    INSTALL_DESTINATION ${FMT_CMAKE_DIR})
  export(TARGETS ${INSTALL_TARGETS} NAMESPACE fmt::
         FILE ${PROJECT_BINARY_DIR}/${targets_export_name}.cmake)

  # Install version, config and target files.
  install(
    FILES ${project_config} ${version_config}
    DESTINATION ${FMT_CMAKE_DIR})
  install(EXPORT ${targets_export_name} DESTINATION ${FMT_CMAKE_DIR}
    NAMESPACE fmt::)

  # Install the library and headers.
  install(TARGETS ${INSTALL_TARGETS} EXPORT ${targets_export_name}
          DESTINATION ${FMT_LIB_DIR})
  install(FILES ${FMT_HEADERS} DESTINATION include/fmt)
endif ()
