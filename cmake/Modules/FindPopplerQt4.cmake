# - Try to find LibPoppler-qt4
# Once done this will define
#  POPPLERQT4_FOUND - System has LibPoppler-qt4
#  POPPLERQT4_INCLUDE_DIRS - The LibPoppler-qt4 include directories
#  POPPLERQT4_LIBRARIES - The libraries needed to use LibPoppler-qt4
#  POPPLERQT4_DEFINITIONS - Compiler switches required for using LibPoppler-qt4

find_package(PkgConfig)
pkg_check_modules(PC_POPPLERQT4 QUIET poppler-qt4)
set(XML2_DEFINITIONS ${PC_POPPLERQT4_CFLAGS_OTHER})

find_path(POPPLERQT4_INCLUDE_DIR poppler-qt4.h
          HINTS ${PC_POPPLERQT4_INCLUDEDIR} ${PC_POPPLERQT4_INCLUDE_DIRS}
          PATH_SUFFIXES poppler/qt4/ )

find_library(POPPLERQT4_LIBRARY NAMES poppler-qt4
             HINTS ${PC_POPPLERQT4_DIR} ${PC_POPPLERQT4_LIBRARY_DIRS} )

set(POPPLERQT4_LIBRARIES ${POPPLERQT4_LIBRARY} )
set(POPPLERQT4_INCLUDE_DIRS ${POPPLERQT4_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set POPPLERQT4_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(PopplerQt4  DEFAULT_MSG
                                  POPPLERQT4_LIBRARY POPPLERQT4_INCLUDE_DIR)

mark_as_advanced(POPPLERQT4_INCLUDE_DIR POPPLERQT4_LIBRARY )