cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_executable (bdj4info bdj4info.c)
if (WIN32)
  target_compile_options (bdj4info PRIVATE -municode)
endif()
target_link_libraries (bdj4info
  libbdj4basic libbdj4common
  ${ICU_LDFLAGS}
)
addIntlLibrary (bdj4info)

add_executable (bdj4tags bdj4tags.c)
if (WIN32)
  target_compile_options (bdj4tags PRIVATE -municode)
endif()
target_link_libraries (bdj4tags
  libbdj4 libbdj4basic libbdj4common
  ${ICU_LDFLAGS}
)
addIntlLibrary (bdj4tags)

install (TARGETS
  bdj4info bdj4tags
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)

if (APPLE)
  macUpdateRPath (bdj4info)
  macUpdateRPath (bdj4tags)
endif()
