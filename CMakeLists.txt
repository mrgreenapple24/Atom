cmake_minimum_required(VERSION 3.16)

project(Atoms)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =====================================================
# GLFW
# =====================================================
add_subdirectory(external/glfw)

# =====================================================
# GLEW
# =====================================================
set(GLEW_USE_STATIC_LIBS ON)

add_subdirectory(external/glew/build/cmake)

# =====================================================
# OpenGL
# =====================================================
find_package(OpenGL REQUIRED)

# =====================================================
# Source files
# =====================================================
file(GLOB_RECURSE SRC_FILES
    src/*.cpp
    src/*.c
)
list(APPEND SRC_FILES external/glad/src/glad.c)
list(APPEND SRC_FILES src/Font.cpp)

# =====================================================
# Executable
# =====================================================
add_executable(${PROJECT_NAME}
    ${SRC_FILES}
)

# =====================================================
# Include directories
# =====================================================
target_include_directories(${PROJECT_NAME} PUBLIC
    include
    external/glm
    external/glad/include
)

# =====================================================
# Link libraries
# =====================================================
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    glfw
    OpenGL::GL
)

# =====================================================
# Linux dependencies
# =====================================================
if(UNIX AND NOT APPLE)
  target_link_libraries(${PROJECT_NAME}
        PRIVATE
        dl
        pthread
        X11
        Xrandr
        Xi
        Xxf86vm
        Xcursor
    )
endif()

# =====================================================
# Compiler warnings
# =====================================================
if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
  target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall
        -Wextra
        -pedantic
    )
endif()
