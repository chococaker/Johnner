set(PROJECT_SOURCES
    board.cpp
    engine.cpp
    macros.cpp
)

# Create a small library for shared code (no need for include/)
add_library(core STATIC ${PROJECT_SOURCES})

# Since headers live in the same folder, expose this folder as include path:
target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# --- Executable 1 ---
add_executable(johnner_fard main.cpp)
target_link_libraries(johnner_fard PRIVATE core)

# --- Executable 2 ---
# add_executable(app2 main_app2.cpp)
# target_link_libraries(app2 PRIVATE core)
