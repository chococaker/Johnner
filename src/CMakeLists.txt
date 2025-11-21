set(PROJECT_SOURCES
    board.cpp
    search.cpp
    macros.cpp
    perft.cpp
    types.cpp
)

add_library(core STATIC ${PROJECT_SOURCES})

target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(johnner main.cpp)
target_link_libraries(johnner PRIVATE core)
add_executable(johnner_perft perft_test.cpp)
target_link_libraries(johnner_perft PRIVATE core)
