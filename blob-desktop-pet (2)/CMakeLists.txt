cmake_minimum_required(VERSION 3.10)
project(blob CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
find_package(X11 REQUIRED)

add_executable(blob
    src/main.cpp
    src/Application.cpp
    src/DesktopWindow.cpp
    src/ScreenInfo.cpp
    src/MovementController.cpp
    src/EmotionSystem.cpp
    src/Eye.cpp
    src/BodyAnimation.cpp
    src/Effects.cpp
    src/Blob.cpp
    src/BehaviorController.cpp
    src/CursorTracker.cpp
    src/CommandInterface.cpp
)

target_include_directories(blob PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(blob PRIVATE
    ${SDL2_LIBRARIES}
    ${X11_LIBRARIES}
    ${X11_Xext_LIB}
    Xrandr
)
target_link_directories(blob PRIVATE ${SDL2_LIBRARY_DIRS})
target_compile_options(blob PRIVATE ${SDL2_CFLAGS_OTHER})
