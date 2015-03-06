# - Try to find LibPoppler-qt5
# Once done this will define
#  POPPLERQT5_FOUND - System has LibPoppler-qt5
#  POPPLERQT5_INCLUDE_DIRS - The LibPoppler-qt5 include directories
#  POPPLERQT5_LIBRARIES - The libraries needed to use LibPoppler-qt5
#  POPPLERQT5_DEFINITIONS - Compiler switches required for using LibPoppler-qt5

find_package(PkgConfig)
pkg_check_modules(PC_POPPLERQT5 QUIET poppler-qt5)
set(XML2_DEFINITIONS ${PC_POPPLERQT5_CFLAGS_OTHER})

find_path(POPPLERQT5_INCLUDE_DIR poppler-qt5.h
          HINTS ${PC_POPPLERQT5_INCLUDEDIR} ${PC_POPPLERQT5_INCLUDE_DIRS}
          PATH_SUFFIXES poppler/qt5/ )

find_library(POPPLERQT5_LIBRARY NAMES poppler-qt5
             HINTS ${PC_POPPLERQT5_DIR} ${PC_POPPLERQT5_LIBRARY_DIRS} )

set(POPPLERQT5_LIBRARIES ${POPPLERQT5_LIBRARY} )
set(POPPLERQT5_INCLUDE_DIRS ${POPPLERQT5_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set POPPLERQT5_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(PopplerQt5  DEFAULT_MSG
                                  POPPLERQT5_LIBRARY POPPLERQT5_INCLUDE_DIR)

mark_as_advanced(POPPLERQT5_INCLUDE_DIR POPPLERQT5_LIBRARY )