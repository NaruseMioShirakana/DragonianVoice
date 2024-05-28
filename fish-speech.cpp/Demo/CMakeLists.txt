include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(EXE_DEMO Demo)
add_compile_options(/ZI)
add_executable(${EXE_DEMO} main.cpp)
target_link_libraries(${EXE_DEMO} PRIVATE fish-speech-cpp)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ${EXE_DEMO} PROPERTY CXX_STANDARD 20)
endif()