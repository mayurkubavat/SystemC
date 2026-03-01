option(USE_RTL "Use RTL (Verilog) instead of SystemC model" OFF)

add_executable(dma_sim top/top.cpp)

target_include_directories(dma_sim PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/rtl
    ${CMAKE_CURRENT_SOURCE_DIR}/env
    ${CMAKE_CURRENT_SOURCE_DIR}/agents
    ${CMAKE_CURRENT_SOURCE_DIR}/sequences
    ${CMAKE_CURRENT_SOURCE_DIR}/test
    # Reuse APB components from sibling project
    ${CMAKE_SOURCE_DIR}/projects/apb/rtl
    ${CMAKE_SOURCE_DIR}/projects/apb/env
    ${CMAKE_SOURCE_DIR}/projects/apb/agents
    ${CMAKE_SOURCE_DIR}/projects/apb/sequences
)

target_link_libraries(dma_sim PRIVATE
    SystemC::systemc
    SystemC::uvm
)

if(USE_RTL)
    find_package(verilator REQUIRED)

    verilate(dma_sim
        SYSTEMC
        SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/rtl/verilog/dma_engine.v
    )

    target_compile_definitions(dma_sim PRIVATE USE_RTL=1)
    message(STATUS "DMA: using RTL DUT (via Verilator)")
else()
    message(STATUS "DMA: using SystemC model DUT")
endif()
