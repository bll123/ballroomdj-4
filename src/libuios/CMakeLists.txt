#
# Copyright 2021-2026 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

include (../utils/bdj4macros.cmake)

#add_library (objuios SHARED)
if (SYSMACOS)
  add_library (objuios OBJECT osmacosuiutils.m)
endif()
if (SYSWIN)
  add_library (objuios OBJECT oswinuiutils.m)
endif()
if (SYSLINUX)
  add_library (objuios OBJECT oslinuxuiutils.c)
#  target_sources (objuios PRIVATE oslinuxuiutils.c)
endif()
addUIInclude (objuios)
