cmake_minimum_required(VERSION 3.15)
project(OpenGL-Raytracer)

set(CMAKE_CXX_STANDARD 17)

# Find packages
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm REQUIRED)

# ImGui
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/src/vendor)
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)
# Use GLEW as the GL loader for ImGui OpenGL3 backend
target_compile_definitions(imgui PRIVATE IMGUI_IMPL_OPENGL_LOADER_GLEW)
target_link_libraries(imgui PUBLIC glfw GLEW::GLEW opengl32)

# Add source files
set(SOURCES
    src/main.cpp
    src/renderer/camera.cpp
    src/renderer/shader_class.cpp
    src/renderer/stb_image.cpp
    src/renderer/renderer.cpp
    src/engine/engine.cpp
    src/engine/ui_handler.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    glfw
    GLEW::GLEW
    glm::glm
    opengl32
    imgui
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

# Set source and destination directories
set(SHADER_SOURCE_DIR ${CMAKE_SOURCE_DIR}/res)
set(SHADER_BUILD_DIR ${CMAKE_BINARY_DIR}/res)

# Gather all shader files (add more extensions if needed)
file(GLOB SHADER_FILES "${SHADER_SOURCE_DIR}/*.shader")

# Custom command: clear build shader dir, then copy all shaders
add_custom_command(
    OUTPUT ${SHADER_BUILD_DIR}/.shaders_copied
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${SHADER_BUILD_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_BUILD_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${SHADER_SOURCE_DIR} ${SHADER_BUILD_DIR}
    COMMAND ${CMAKE_COMMAND} -E touch ${SHADER_BUILD_DIR}/.shaders_copied
    DEPENDS ${SHADER_FILES}
    COMMENT "Refreshing shaders in build directory"
)

add_custom_target(refresh_shaders ALL
    DEPENDS ${SHADER_BUILD_DIR}/.shaders_copied
)

# Make your executable depend on the shaders being refreshed
add_dependencies(${PROJECT_NAME} refresh_shaders)