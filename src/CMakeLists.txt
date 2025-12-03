set(PROJECT_SOURCES
    board.cpp
    search.cpp
    macros.cpp
    movegen.cpp
    perft.cpp
    types.cpp
    uci.cpp
    zobrist.cpp
)

add_library(core STATIC ${PROJECT_SOURCES})

target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(johnner_v2 main.cpp)
target_link_libraries(johnner_v2 PRIVATE core)
