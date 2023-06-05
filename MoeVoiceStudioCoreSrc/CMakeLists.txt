cmake_minimum_required(VERSION 3.12)
project(MoeVoiceStudio VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置 OnnxRuntime 版本
set(ONNXRUNTIME_VERSION 1.15.0)

if(WIN32)
    set(ONNXRUNTIME_DOWNLOAD_URL https://github.com/microsoft/onnxruntime/releases/download/v${ONNXRUNTIME_VERSION}/onnxruntime-win-x64-${ONNXRUNTIME_VERSION}.zip)
    set(ONNXRUNTIME_ARCHIVE ${PROJECT_SOURCE_DIR}/onnxruntime.zip)
    set(ONNXRUNTIME_LIB_DIR ${PROJECT_SOURCE_DIR}/onnxruntime/lib)
    set(EXPAND_COMMAND powershell -command "Expand-Archive -Force -Path ${ONNXRUNTIME_ARCHIVE} -DestinationPath ${PROJECT_SOURCE_DIR}/onnxruntime")
elseif(APPLE)
    # 你需要找到对应的macOS版本的ONNXRUNTIME下载链接
    # 这里的路径可能需要修改，取决于实际的解压文件结构
    set(ONNXRUNTIME_LIB_DIR ${PROJECT_SOURCE_DIR}/onnxruntime/lib)
    set(EXPAND_COMMAND tar -xzf ${ONNXRUNTIME_ARCHIVE} -C ${PROJECT_SOURCE_DIR})
else() # Assuming Linux
    set(ONNXRUNTIME_DOWNLOAD_URL https://github.com/microsoft/onnxruntime/releases/download/v${ONNXRUNTIME_VERSION}/onnxruntime-linux-x64-${ONNXRUNTIME_VERSION}.tgz)
    set(ONNXRUNTIME_ARCHIVE ${PROJECT_SOURCE_DIR}/onnxruntime.tgz)

    # 这里的路径可能需要修改，取决于实际的解压文件结构
    set(ONNXRUNTIME_LIB_DIR ${PROJECT_SOURCE_DIR}/onnxruntime/lib)
    set(EXPAND_COMMAND tar -xzf ${ONNXRUNTIME_ARCHIVE} -C ${PROJECT_SOURCE_DIR})
endif()

if(NOT EXISTS "${ONNXRUNTIME_ARCHIVE}")
    message(STATUS "OnnxRuntime not found, downloading...")
    file(DOWNLOAD ${ONNXRUNTIME_DOWNLOAD_URL} "${ONNXRUNTIME_ARCHIVE}")
endif()

if(NOT EXISTS "${PROJECT_SOURCE_DIR}/onnxruntime")
    message(STATUS "Extracting OnnxRuntime...")
    execute_process(COMMAND ${EXPAND_COMMAND})
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_subdirectory(Lib/World)

# 添加模块的头文件路径
include_directories(${PROJECT_SOURCE_DIR}/Modules/AvCodec)
include_directories(${PROJECT_SOURCE_DIR}/Modules/InferTools)
include_directories(${PROJECT_SOURCE_DIR}/Modules/Logger)
include_directories(${PROJECT_SOURCE_DIR}/Modules/Models)
include_directories(${PROJECT_SOURCE_DIR}/Modules/PluginApi)
include_directories(${PROJECT_SOURCE_DIR}/Modules/DataStruct)
include_directories(${PROJECT_SOURCE_DIR}/Modules/Models/header)
include_directories(${PROJECT_SOURCE_DIR}/MainWindow)

# 添加库的头文件路径
include_directories(${PROJECT_SOURCE_DIR}/Lib/ffmpeg-4.2.1/include)
include_directories(${PROJECT_SOURCE_DIR}/Lib/rapidjson)
include_directories(${PROJECT_SOURCE_DIR}/Lib/World/src)
include_directories(${PROJECT_SOURCE_DIR}/onnxruntime/include)

# 添加库的链接路径
link_directories(${PROJECT_SOURCE_DIR}/Lib/ffmpeg-4.2.1/Lib)
link_directories(${ONNXRUNTIME_LIB_DIR})
link_directories(${PROJECT_SOURCE_DIR}/Lib/World)

function(moevs_add_files _out)
    file(GLOB_RECURSE _tmp ${ARGN})
    list(APPEND ${_out} ${_tmp})
    set(${_out} ${${_out}} PARENT_SCOPE)
endfunction()

# 找到所有的源文件和头文件
set(PROJECT_SOURCES)
moevs_add_files(PROJECT_SOURCES
    Modules/*.hpp
    Modules/*.cpp
)

# 添加主文件
add_executable(${PROJECT_NAME}
            ${PROJECT_SOURCES}
            main.cpp
        )


# 链接
target_link_libraries(${PROJECT_NAME} PRIVATE
    world
    avcodec.lib
    avformat.lib
    avutil.lib
    swresample.lib
    swscale.lib
    onnxruntime.lib
)
