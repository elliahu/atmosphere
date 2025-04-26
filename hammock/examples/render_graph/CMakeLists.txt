# Add the executable
add_executable(render_graph
        main.cpp
)

# Link the engine library
target_link_libraries(render_graph PRIVATE hammock)
target_include_directories(render_graph PRIVATE ${PROJECT_SOURCE_DIR}/include)