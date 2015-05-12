# Initializes block variables
INIT_BIICODE_BLOCK()

# Actually create targets: EXEcutables and libraries.
ADD_BIICODE_TARGETS()

target_include_directories(${BII_BLOCK_TARGET} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

if (HAVE_OPEN)
  target_compile_definitions(${BII_BLOCK_TARGET} INTERFACE -DFMT_USE_FILE_DESCRIPTORS=1)
endif ()

if (CMAKE_COMPILER_IS_GNUCXX)
  target_compile_options(${BII_BLOCK_TARGET} INTERFACE -Wall -Wextra -Wshadow -pedantic)
endif ()
if (CPP11_FLAG AND FMT_PEDANTIC)
  target_compile_options(${BII_BLOCK_TARGET} INTERFACE ${CPP11_FLAG})
endif ()
