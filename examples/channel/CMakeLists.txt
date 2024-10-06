cmake_minimum_required(VERSION 3.16.3)
project(channel VERSION 1 LANGUAGES CXX)

file(GLOB IPC_EXAMPLE_SOURCES_CXX "*.cxx")
file(GLOB IPC_EXAMPLE_SOURCES_CPP "*.cpp")

add_executable(${PROJECT_NAME} ${IPC_EXAMPLE_SOURCES_CXX} ${IPC_EXAMPLE_SOURCES_CPP})
target_include_directories(${PROJECT_NAME}
  PUBLIC  
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/../../include>
)

target_link_libraries(${PROJECT_NAME} 
    tiny-ipc
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
    $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
)
