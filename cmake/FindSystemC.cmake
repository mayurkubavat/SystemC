# FindSystemC.cmake
# Provides: SystemC::systemc, SystemC::uvm (optional)

# Detect platform library directory
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin" AND CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
    set(_SC_LIB_SUFFIX "lib-macosarm64")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(_SC_LIB_SUFFIX "lib-macosx64")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(_SC_LIB_SUFFIX "lib-linux64")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(_SC_LIB_SUFFIX "lib-linux")
else()
    set(_SC_LIB_SUFFIX "lib")
endif()

# Find SystemC
if(NOT SYSTEMC_HOME)
    set(SYSTEMC_HOME $ENV{SYSTEMC_HOME})
endif()

find_path(SystemC_INCLUDE_DIR NAMES systemc.h
    PATHS ${SYSTEMC_HOME}/include NO_DEFAULT_PATH)

find_library(SystemC_LIBRARY NAMES systemc
    PATHS ${SYSTEMC_HOME}/${_SC_LIB_SUFFIX} ${SYSTEMC_HOME}/lib NO_DEFAULT_PATH)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SystemC REQUIRED_VARS SystemC_LIBRARY SystemC_INCLUDE_DIR)

if(SystemC_FOUND AND NOT TARGET SystemC::systemc)
    add_library(SystemC::systemc UNKNOWN IMPORTED)
    set_target_properties(SystemC::systemc PROPERTIES
        IMPORTED_LOCATION "${SystemC_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${SystemC_INCLUDE_DIR}")
endif()

# Find UVM-SystemC (optional)
if(NOT UVM_SYSTEMC_HOME)
    set(UVM_SYSTEMC_HOME $ENV{UVM_SYSTEMC_HOME})
endif()

if(UVM_SYSTEMC_HOME)
    find_path(UVM_SystemC_INCLUDE_DIR NAMES uvm.h
        PATHS ${UVM_SYSTEMC_HOME}/include NO_DEFAULT_PATH)
    find_library(UVM_SystemC_LIBRARY NAMES uvm-systemc
        PATHS ${UVM_SYSTEMC_HOME}/${_SC_LIB_SUFFIX} ${UVM_SYSTEMC_HOME}/lib NO_DEFAULT_PATH)
    if(UVM_SystemC_INCLUDE_DIR AND UVM_SystemC_LIBRARY)
        set(UVM_SystemC_FOUND TRUE)
        if(NOT TARGET SystemC::uvm)
            add_library(SystemC::uvm UNKNOWN IMPORTED)
            set_target_properties(SystemC::uvm PROPERTIES
                IMPORTED_LOCATION "${UVM_SystemC_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${UVM_SystemC_INCLUDE_DIR}"
                INTERFACE_LINK_LIBRARIES SystemC::systemc)
        endif()
    endif()
endif()
