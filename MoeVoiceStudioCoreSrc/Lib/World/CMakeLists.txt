project(WORLD LANGUAGES CXX)

add_library(world STATIC
    src/world/cheaptrick.h
    src/world/codec.h
    src/world/common.h
    src/world/constantnumbers.h
    src/world/d4c.h
    src/world/dio.h
    src/world/fft.h
    src/world/harvest.h
    src/world/macrodefinitions.h
    src/world/matlabfunctions.h
    src/world/stonemask.h
    src/world/synthesis.h
    src/world/synthesisrealtime.h
    src/cheaptrick.cpp
    src/codec.cpp
    src/common.cpp
    src/d4c.cpp
    src/dio.cpp
    src/fft.cpp
    src/harvest.cpp
    src/matlabfunctions.cpp
    src/stonemask.cpp
    src/synthesis.cpp
    src/synthesisrealtime.cpp
)

add_library(world_tool STATIC
    tools/audioio.h
    tools/parameterio.h
    tools/audioio.cpp
    tools/parameterio.cpp
)

add_library(world::core ALIAS world)
add_library(world::tool ALIAS world_tool)

target_link_libraries(world_tool PUBLIC world)

foreach(lib world world_tool)
    target_include_directories(${lib} PUBLIC $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> PRIVATE src)
    set_target_properties(${lib}
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endforeach()