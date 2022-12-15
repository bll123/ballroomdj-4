cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_executable (bdj4info bdj4info.c)
if (WIN32)
  target_compile_options (bdj4info PRIVATE -municode)
endif()
target_link_libraries (bdj4info
  libbdj4basic libbdj4common
  ${ICU_LDFLAGS}
  m
)
addIntlLibrary (bdj4info)

add_executable (bdj4tags bdj4tags.c)
if (WIN32)
  target_compile_options (bdj4tags PRIVATE -municode)
endif()
target_link_libraries (bdj4tags
  libbdj4 libbdj4basic libbdj4common
  ${ICU_LDFLAGS}
  m
)
addIntlLibrary (bdj4tags)

add_executable (tdbcompare tdbcompare.c)
if (WIN32)
  target_compile_options (tdbcompare PRIVATE -municode)
endif()
target_link_libraries (tdbcompare
  libbdj4 libbdj4basic libbdj4common
  ${ICU_LDFLAGS}
  m
)

add_executable (tdbdump tdbdump.c)
if (WIN32)
  target_compile_options (tdbdump PRIVATE -municode)
endif()
target_link_libraries (tdbdump
  libbdj4 libbdj4basic libbdj4common
  ${ICU_LDFLAGS}
  m
)

add_executable (testsuite
  testsuite.c
)
target_link_libraries (testsuite
  libbdj4 libbdj4basic libbdj4common
  ${ICU_LDFLAGS}
  m
)

add_executable (tmusicsetup tmusicsetup.c)
if (WIN32)
  target_compile_options (tmusicsetup PRIVATE -municode)
endif()
target_link_libraries (tmusicsetup
  libbdj4 libbdj4basic libbdj4common
  ${ICU_LDFLAGS}
  m
)
addIntlLibrary (tmusicsetup)

add_executable (vlcsinklist vlcsinklist.c)
target_link_libraries (vlcsinklist
  libbdj4pli libbdj4vol libbdj4basic libbdj4common
  m
)

add_executable (vsencdec vsencdec.c)
target_link_libraries (vsencdec
  libbdj4common
  m
)

install (TARGETS
  bdj4info bdj4tags tdbcompare tdbdump testsuite tmusicsetup
  vlcsinklist vsencdec
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)

if (APPLE)
  macUpdateRPath (bdj4info)
  macUpdateRPath (bdj4tags)
  macUpdateRPath (tdbcompare)
  macUpdateRPath (tdbdump)
  macUpdateRPath (testsuite)
  macUpdateRPath (tmusicsetup)
  macUpdateRPath (vlcsinklist)
  macUpdateRPath (vsencdec)
endif()
