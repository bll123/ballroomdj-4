#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

add_executable (bdj4info bdj4info.c)
if (WIN32)
  target_compile_options (bdj4info PRIVATE -municode)
endif()
target_include_directories (bdj4info
  PUBLIC ${GTK_INCLUDE_DIRS}
)
target_link_libraries (bdj4info PUBLIC
  libbdj4basic libbdj4common
  ${ICU_LDFLAGS}
  m
)
addIntlLibrary (bdj4info)
updateRPath (bdj4info)

add_executable (bdj4tags bdj4tags.c)
if (WIN32)
  target_compile_options (bdj4tags PRIVATE -municode)
endif()
target_link_libraries (bdj4tags PUBLIC
  libbdj4ati libbdj4 libbdj4basic libbdj4common
  ${ICU_LDFLAGS}
  z
  m
)
addIntlLibrary (bdj4tags)
updateRPath (bdj4tags)

add_executable (tdbcompare tdbcompare.c)
if (WIN32)
  target_compile_options (tdbcompare PRIVATE -municode)
endif()
target_link_libraries (tdbcompare PUBLIC
  libbdj4ati libbdj4 libbdj4basic libbdj4common
  ${ICU_LDFLAGS}
  z
  m
)

add_executable (tdbsetval tdbsetval.c)
if (WIN32)
  target_compile_options (tdbsetval PRIVATE -municode)
endif()
target_link_libraries (tdbsetval PUBLIC
  libbdj4ati libbdj4 libbdj4basic libbdj4common
  ${ICU_LDFLAGS}
  z
  m
)
updateRPath (tdbsetval)

add_executable (testsuite
  testsuite.c
)
target_link_libraries (testsuite PUBLIC
  libbdj4 libbdj4basic libbdj4common
  ${ICU_LDFLAGS}
  z
  m
)
updateRPath (testsuite)

add_executable (tmusicsetup tmusicsetup.c)
if (WIN32)
  target_compile_options (tmusicsetup PRIVATE -municode)
endif()
target_link_libraries (tmusicsetup PUBLIC
  libbdj4ati libbdj4 libbdj4basic libbdj4common
  ${ICU_LDFLAGS}
  z
  m
)
addIntlLibrary (tmusicsetup)
updateRPath (tmusicsetup)

add_executable (ttagdbchk ttagdbchk.c)
if (WIN32)
  target_compile_options (ttagdbchk PRIVATE -municode)
endif()
target_link_libraries (ttagdbchk PUBLIC
  libbdj4ati libbdj4 libbdj4basic libbdj4common
  ${ICU_LDFLAGS}
  z
  m
)
updateRPath (ttagdbchk)

add_executable (vlcsinklist vlcsinklist.c)
target_link_libraries (vlcsinklist PUBLIC
  libbdj4pli libbdj4vol libbdj4basic libbdj4common
  ${ICU_LDFLAGS}
  m
)
updateRPath (vlcsinklist)

add_executable (vsed vsed.c)
target_link_libraries (vsed PUBLIC
  libbdj4common
  m
)
updateRPath (vsed)

install (TARGETS
  bdj4info bdj4tags tdbcompare tdbsetval testsuite tmusicsetup ttagdbchk
  vlcsinklist vsed
  DESTINATION ${PROJECT_SOURCE_DIR}/../bin
)
