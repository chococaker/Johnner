set(PROJECT_SOURCES
    board.cpp
    search.cpp
    macros.cpp
    perft.cpp
    types.cpp
    uci.cpp
)

add_library(core STATIC ${PROJECT_SOURCES})

target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(johnner_perft perft_test.cpp)
target_link_libraries(johnner_perft PRIVATE core)
add_executable(johnner_uci main_uci.cpp)
target_link_libraries(johnner_uci PRIVATE core)
