cmake_minimum_required(VERSION 3.12)
project(MoeVoice)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)

set(ONNXRUNTIME_VERSION 1.15.0)
set(PROJECT_CORE_SOURCE ../MoeVoiceStudioCoreSrc)
get_filename_component(PROJECT_CORE_SOURCE_DIR "${PROJECT_CORE_SOURCE}" ABSOLUTE)
message(STATUS "Directory (Absolute): ${PROJECT_CORE_SOURCE_DIR}")

if(WIN32)
  set(ONNXRUNTIME_DOWNLOAD_URL https://github.com/microsoft/onnxruntime/releases/download/v${ONNXRUNTIME_VERSION}/onnxruntime-win-x64-${ONNXRUNTIME_VERSION}.zip)
  set(ONNXRUNTIME_ARCHIVE ${PROJECT_CORE_SOURCE_DIR}/onnxruntime.zip)
  set(ONNXRUNTIME_LIB_DIR ${PROJECT_CORE_SOURCE_DIR}/onnxruntime/onnxruntime-win-x64-${ONNXRUNTIME_VERSION}/lib)
  set(ONNXRUNTIME_HEAD_DIR ${PROJECT_CORE_SOURCE_DIR}/onnxruntime/onnxruntime-win-x64-${ONNXRUNTIME_VERSION}/include)
  set(EXPAND_COMMAND powershell -command "Expand-Archive -Force -Path ${ONNXRUNTIME_ARCHIVE} -DestinationPath ${PROJECT_CORE_SOURCE_DIR}/onnxruntime")
elseif(APPLE)
  # 你需要找到对应的macOS版本的ONNXRUNTIME下载链接
  # 这里的路径可能需要修改，取决于实际的解压文件结构
  set(ONNXRUNTIME_LIB_DIR ${PROJECT_CORE_SOURCE_DIR}/onnxruntime/lib)
  set(ONNXRUNTIME_HEAD_DIR ${PROJECT_CORE_SOURCE_DIR}/onnxruntime/include)
  set(EXPAND_COMMAND tar -xzf ${ONNXRUNTIME_ARCHIVE} -C ${PROJECT_CORE_SOURCE_DIR})
else()  # Assuming Linux
  set(ONNXRUNTIME_DOWNLOAD_URL https://github.com/microsoft/onnxruntime/releases/download/v${ONNXRUNTIME_VERSION}/onnxruntime-linux-x64-${ONNXRUNTIME_VERSION}.tgz)
  set(ONNXRUNTIME_ARCHIVE ${PROJECT_CORE_SOURCE_DIR}/onnxruntime.tgz)
  # 这里的路径可能需要修改，取决于实际的解压文件结构
  set(ONNXRUNTIME_LIB_DIR ${PROJECT_CORE_SOURCE_DIR}/onnxruntime/lib)
  set(ONNXRUNTIME_HEAD_DIR ${PROJECT_CORE_SOURCE_DIR}/onnxruntime/include)
  set(EXPAND_COMMAND tar -xzf ${ONNXRUNTIME_ARCHIVE} -C ${PROJECT_CORE_SOURCE_DIR})
endif()

if(NOT EXISTS "${ONNXRUNTIME_ARCHIVE}")
  file(DOWNLOAD ${ONNXRUNTIME_DOWNLOAD_URL} "${ONNXRUNTIME_ARCHIVE}")
endif()

if(NOT EXISTS "${PROJECT_CORE_SOURCE_DIR}/onnxruntime")
  execute_process(COMMAND ${EXPAND_COMMAND})
endif()

if(MSVC)
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
endif()


# 添加模块的头文件路径
include_directories(${PROJECT_CORE_SOURCE_DIR}/Modules/AvCodec)
include_directories(${PROJECT_CORE_SOURCE_DIR}/Modules/InferTools)
include_directories(${PROJECT_CORE_SOURCE_DIR}/Modules/Logger)
include_directories(${PROJECT_CORE_SOURCE_DIR}/Modules/Models)
include_directories(${PROJECT_CORE_SOURCE_DIR}/Modules/PluginApi)
include_directories(${PROJECT_CORE_SOURCE_DIR}/Modules/DataStruct)


# 添加库的头文件路径
include_directories(${PROJECT_CORE_SOURCE_DIR}/Lib/ffmpeg-4.2.1/include)
include_directories(${PROJECT_CORE_SOURCE_DIR}/Lib/rapidjson)
include_directories(${PROJECT_CORE_SOURCE_DIR}/Lib/World/src)
include_directories(${ONNXRUNTIME_HEAD_DIR})

# 添加库的链接路径
link_directories(${PROJECT_CORE_SOURCE_DIR}/Lib/ffmpeg-4.2.1/Lib)
link_directories(${ONNXRUNTIME_LIB_DIR})
link_directories(${PROJECT_CORE_SOURCE_DIR}/Lib/World)

# 找到所有的源文件和头文件
file(GLOB_RECURSE SRCS "${PROJECT_CORE_SOURCE_DIR}/Modules/*.cpp")
file(GLOB_RECURSE HDRS "${PROJECT_CORE_SOURCE_DIR}/Modules/*.hpp")

# 添加动态库
add_library(MoeVoiceLib SHARED ${SRCS})
target_link_libraries(MoeVoiceLib World.lib avcodec.lib avformat.lib avutil.lib swresample.lib swscale.lib onnxruntime.lib)

# 添加主文件
add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} MoeVoiceLib)