option(USE_RTL "Use RTL (Verilog) instead of SystemC model" OFF)

add_executable(sim top/top.cpp)

target_include_directories(sim PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/rtl
    ${CMAKE_CURRENT_SOURCE_DIR}/env
    ${CMAKE_CURRENT_SOURCE_DIR}/agents
    ${CMAKE_CURRENT_SOURCE_DIR}/sequences
    ${CMAKE_CURRENT_SOURCE_DIR}/test
)

target_link_libraries(sim PRIVATE
    SystemC::systemc
    SystemC::uvm
)

if(USE_RTL)
    find_package(verilator REQUIRED)

    verilate(sim
        SYSTEMC
        SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/rtl/verilog/apb_slave.v
    )

    target_compile_definitions(sim PRIVATE USE_RTL=1)
    message(STATUS "APB: using RTL DUT (via Verilator)")
else()
    message(STATUS "APB: using SystemC model DUT")
endif()
