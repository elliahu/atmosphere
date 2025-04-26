# Collect all source and header files
file(GLOB_RECURSE SOURCE_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

# Add the executable
add_executable(environment_maps_generator
        ${SOURCE_FILES}
)

# Link the engine library
target_link_libraries(environment_maps_generator PRIVATE hammock)
target_include_directories(environment_maps_generator PRIVATE ${PROJECT_SOURCE_DIR}/include)