cmake_minimum_required(VERSION 3.29)
project(hammock)

# Set global C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Legacy parts of the code missmatch new[] and free() so comment this if sanitizer crashes
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g")

# Add ImGui source files
file(GLOB_RECURSE IMGUI_SRC
        ${PROJECT_SOURCE_DIR}/external/imgui/*.cpp
)

add_subdirectory(src)

# Define engine library
add_library(hammock STATIC ${HAMMOCK_SRC} ${IMGUI_SRC})

# Include directories for engine
target_include_directories(hammock PUBLIC ${PROJECT_SOURCE_DIR}/include/)
target_include_directories(hammock PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(hammock PUBLIC $ENV{VULKAN_SDK}/Include)
target_include_directories(hammock PUBLIC ${PROJECT_SOURCE_DIR}/external)
target_include_directories(hammock PUBLIC ${PROJECT_SOURCE_DIR}/external/imgui)

if(WIN32)
    target_compile_definitions(hammock PRIVATE VK_USE_PLATFORM_WIN32_KHR)
    target_link_libraries(hammock PRIVATE Dwmapi)

elseif(UNIX AND NOT APPLE)
    target_compile_definitions(hammock PRIVATE VK_USE_PLATFORM_XLIB_KHR)

    # Find and link X11 libraries
    find_package(X11 REQUIRED)
    target_include_directories(hammock PRIVATE ${X11_INCLUDE_DIR})
    target_link_libraries(hammock PRIVATE ${X11_LIBRARIES})
    target_link_libraries(hammock PRIVATE ${X11_LIBRARIES} X11::Xkb)
    
elseif(APPLE)
    target_compile_definitions(hammock PRIVATE VK_USE_PLATFORM_METAL_EXT)
endif()

# Link dependencies
find_package(Vulkan REQUIRED )
target_link_libraries(hammock PUBLIC Vulkan::Vulkan)

# Also build examples
# add_subdirectory(examples)

# And also build tools
# add_subdirectory(tools)


