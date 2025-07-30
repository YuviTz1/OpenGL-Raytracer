cmake_minimum_required(VERSION 3.15)
project(OpenGL-Raytracer)

set(CMAKE_CXX_STANDARD 17)

# Find packages
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm REQUIRED)

# Add source files
set(SOURCES
    src/main.cpp
    src/shader_class.cpp
    src/stb_image.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME} 
    glfw
    GLEW::GLEW
    glm::glm
    opengl32
)

# Copy the entire res directory to the output directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/res
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/res
)

# Set working directory to build directory (so shaders can be found)
set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)