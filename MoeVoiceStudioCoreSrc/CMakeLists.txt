cmake_minimum_required(VERSION 3.12)
project(MoeVoiceStudio VERSION 0.0.1 LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(ONNXRUNTIME_VERSION 1.15.0)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools)

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
else()  # Assuming Linux
  set(ONNXRUNTIME_DOWNLOAD_URL https://github.com/microsoft/onnxruntime/releases/download/v${ONNXRUNTIME_VERSION}/onnxruntime-linux-x64-${ONNXRUNTIME_VERSION}.tgz)
  set(ONNXRUNTIME_ARCHIVE ${PROJECT_SOURCE_DIR}/onnxruntime.tgz)
  # 这里的路径可能需要修改，取决于实际的解压文件结构
  set(ONNXRUNTIME_LIB_DIR ${PROJECT_SOURCE_DIR}/onnxruntime/lib)
  set(EXPAND_COMMAND tar -xzf ${ONNXRUNTIME_ARCHIVE} -C ${PROJECT_SOURCE_DIR})
endif()

if(NOT EXISTS "${ONNXRUNTIME_ARCHIVE}")
  file(DOWNLOAD ${ONNXRUNTIME_DOWNLOAD_URL} "${ONNXRUNTIME_ARCHIVE}")
endif()

if(NOT EXISTS "${PROJECT_SOURCE_DIR}/onnxruntime")
  execute_process(COMMAND ${EXPAND_COMMAND})
endif()

# 添加模块的头文件路径
include_directories(${PROJECT_SOURCE_DIR}/Modules/AvCodec)
include_directories(${PROJECT_SOURCE_DIR}/Modules/InferTools)
include_directories(${PROJECT_SOURCE_DIR}/Modules/Logger)
include_directories(${PROJECT_SOURCE_DIR}/Modules/Models)
include_directories(${PROJECT_SOURCE_DIR}/Modules/PluginApi)
include_directories(${PROJECT_SOURCE_DIR}/Modules/DataStruct)

# 添加库的头文件路径
include_directories(${PROJECT_SOURCE_DIR}/Lib/ffmpeg-4.2.1/include)
include_directories(${PROJECT_SOURCE_DIR}/Lib/rapidjson)
include_directories(${PROJECT_SOURCE_DIR}/Lib/World/src)
include_directories(${PROJECT_SOURCE_DIR}/onnxruntime/include)

# 添加库的链接路径
link_directories(${PROJECT_SOURCE_DIR}/Lib/ffmpeg-4.2.1/Lib)
link_directories(${ONNXRUNTIME_LIB_DIR})
link_directories(${PROJECT_SOURCE_DIR}/Lib/World)

# 找到所有的源文件和头文件
file(GLOB_RECURSE SRCS "${PROJECT_SOURCE_DIR}/Modules/*.cpp")
file(GLOB_RECURSE HDRS "${PROJECT_SOURCE_DIR}/Modules/*.hpp")

# QT相关
set(TS_FILES_M localization/MoeVS_zh_CN.ts)

set(PROJECT_SOURCES
        main.cpp
        MainWindow/moevoicestudio.cpp
        MainWindow/moevoicestudio.h
        MainWindow/moevoicestudio.ui
        MainWindow/mainmenu.h
        MainWindow/mainmenu.cpp 
        MainWindow/mainmenu.ui
        MainWindow/SVC/svcmainwindow.h 
        MainWindow/SVC/svcmainwindow.cpp 
        MainWindow/SVC/svcmainwindow.ui
        MainWindow/TTS/ttsmainwindow.h 
        MainWindow/TTS/ttsmainwindow.cpp 
        MainWindow/TTS/ttsmainwindow.ui
        MainWindow/SVS/svsmainwindow.h 
        MainWindow/SVS/svsmainwindow.cpp 
        MainWindow/SVS/svsmainwindow.ui
        ${TS_FILES_M}
        MResource.qrc
)

# 添加主文件
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME}
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        ${SRCS}
    )
    qt_add_translations(${PROJECT_NAME} TS_FILES ${TS_FILES_M})
else()
    if(ANDROID)
        add_library(${PROJECT_NAME} SHARED
            ${PROJECT_SOURCES}
            ${SRCS}
        )
    else()
        add_executable(${PROJECT_NAME}
            ${PROJECT_SOURCES}
            ${SRCS}
        )
    endif()
    qt_add_translations(${PROJECT_NAME} TS_FILES ${TS_FILES_M})
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
	World.lib
	avcodec.lib 
	avformat.lib 
	avutil.lib 
	swresample.lib 
	swscale.lib 
	onnxruntime.lib
    Qt${QT_VERSION_MAJOR}::Widgets
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS ${PROJECT_NAME} 
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()