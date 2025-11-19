set(PROJECT_SOURCES
    board.cpp
    engine.cpp
    macros.cpp
)

add_library(core STATIC ${PROJECT_SOURCES})

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
# set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")

target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(johnner_fard main.cpp)
target_link_libraries(johnner_fard PRIVATE core)
# target_link_options(johnner_fard PRIVATE "-Wl,--subsystem,console")
