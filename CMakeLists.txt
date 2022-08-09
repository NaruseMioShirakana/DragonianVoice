# CMakeList.txt: ShirakanaClass 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required(VERSION 3.16.0)
project ("Shirakana-TTS")


set(CMAKE_CXX_STANDARD 17)
set(VERSION_MAJOR 0)
set(VERSION_MINOR 0)
set(VERSION_PATCH 1)
set(CMAKE_PREFIX_PATH D:/VisualStudioProj/QT/6.3.1/msvc2019_64)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)


find_package(OpenCV REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)
find_package(Qt6 COMPONENTS Multimedia REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS} "D:/VisualStudioProj/onnxruntime-win-x64-gpu-1.9.0/include")

set(QRC_SOURCE_FILES Resources.qrc)
qt6_add_resources(QRC_FILES ${QRC_SOURCE_FILES})
SOURCE_GROUP("Resource Files" FILES ${QRC_SOURCE_FILES})


set(project_ui
	"ShirakanaUI.ui")
set(project_headers
	"ShirakanaTTS-Cpp.h"
	"ShirakanaUI.h")
set(project_sources
	"ShirakanaTTS-Cpp.cpp"
	"ShirakanaUI.cpp")


link_directories(${OpenCV_LIBRARY_DIRS} "D:/VisualStudioProj/onnxruntime-win-x64-gpu-1.9.0/lib")


add_executable(${PROJECT_NAME} ${project_headers} ${project_ui} ${project_sources} ${QRC_FILES} "app_win32.rc")


target_link_libraries(${PROJECT_NAME}
	PUBLIC
	Qt6::Core
	Qt6::Gui
	Qt6::Widgets
    Qt::Multimedia
    ${OpenCV_LIBS}
    "D:/VisualStudioProj/onnxruntime-win-x64-gpu-1.9.0/lib/onnxruntime.lib"
	)



if(WIN32)
        if(MSVC)
                set_target_properties(${PROJECT_NAME} PROPERTIES
                        WIN32_EXECUTABLE YES
                        LINK_FLAGS "/ENTRY:mainCRTStartup"
                )
        elseif(CMAKE_COMPILER_IS_GNUCXX)
                        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mwindows") # Not tested
        else()
                message(SEND_ERROR "You are using an unsupported Windows compiler! (Not MSVC or GCC)")
        endif(MSVC)
elseif(APPLE)
        set_target_properties(${PROJECT_NAME} PROPERTIES
                        MACOSX_BUNDLE YES
        )
elseif(UNIX)
        # Nothing special required
else()
        message(SEND_ERROR "You are on an unsupported platform! (Not Win32, Mac OS X or Unix)")
endif(WIN32)