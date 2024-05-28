cmake_minimum_required (VERSION 3.8)

add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
option(FISHSPEECHCPP_CUDA                    "cuda backend" OFF)
option(FISHSPEECHCPP_ROCM                    "rocm backend" OFF)
option(FISHSPEECHCPP_SHARED_LIBS             "build shared libs" OFF)
option(FISHSPEECHCPP_BUILD_DEMO              "build demo" ON)
add_definitions(-DUNICODE)

set(FishSpeechLib fish-speech-cpp)
set(BUILD_SHARED_LIBS OFF)
if(FISHSPEECHCPP_CUDA)
    
endif()

if (FISHSPEECHCPP_ROCM)
    
endif ()

project (${FishSpeechLib})
set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/out/archive/${CMAKE_BUILD_TYPE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/out/library/${CMAKE_BUILD_TYPE})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/out/runtime/${CMAKE_BUILD_TYPE})
set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/out/lib/${CMAKE_BUILD_TYPE})

if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
message("Build Release")
add_compile_options(/GS- /arch:AVX2 /O2 /Ob2 /Ot)
elseif(${CMAKE_BUILD_TYPE} STREQUAL "RelWithDebInfo")
add_compile_options(/Zi /Gy)
else()
message("Build Debug")
add_definitions(-DFISHSPEECH_DEBUG)
add_compile_options(/Zi /Gy)
endif()

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

function(FISHSPEECHAddFiles _out)
    file(GLOB_RECURSE _tmp ${ARGN})
    list(APPEND ${_out} ${_tmp})
    set(${_out} ${${_out}} PARENT_SCOPE)
endfunction()

set(PROJECT_SOURCES)
FISHSPEECHAddFiles(PROJECT_SOURCES
    ${PROJECT_SOURCE_DIR}/include/*.hpp
    ${PROJECT_SOURCE_DIR}/include/*.h
    ${PROJECT_SOURCE_DIR}/src/*.cpp
    ${PROJECT_SOURCE_DIR}/src/*.c
)
if(FISHSPEECHCPP_SHARED_LIBS)
    add_library(
        ${FishSpeechLib} 
        SHARED ${PROJECT_SOURCES}
    )
    add_definitions(-DFISHSPEECH_EXPORT)
    target_compile_definitions(${FishSpeechLib} PRIVATE -DFISHSPEECH_DLL)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
else()
    add_library(
        ${FishSpeechLib}
        STATIC ${PROJECT_SOURCES}
    )
endif()
message(${PROJECT_SOURCES})

include_directories(${PROJECT_SOURCE_DIR}/include)

add_subdirectory(ggml)
target_link_libraries(${FishSpeechLib} PUBLIC ggml)
include_directories(${PROJECT_SOURCE_DIR}/ggml/src)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ${FishSpeechLib} PROPERTY CXX_STANDARD 23)
endif()

if(FISHSPEECHCPP_BUILD_DEMO)
    add_subdirectory(Demo)
endif()