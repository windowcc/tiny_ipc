project(test-ipc)

if(NOT MSVC)
  add_compile_options(
    -Wno-attributes 
    -Wno-missing-field-initializers 
    -Wno-unused-variable 
    -Wno-unused-function)
endif()
message(${PROJECT_SOURCE_DIR})
include_directories(
    ${PROJECT_SOURCE_DIR}/../include 
    ${PROJECT_SOURCE_DIR}/../src
    ${PROJECT_SOURCE_DIR}/../test
    ${PROJECT_SOURCE_DIR}/../3rdparty
    ${PROJECT_SOURCE_DIR}/../3rdparty/gtest/include)

file(GLOB SRC_FILES
    ${PROJECT_SOURCE_DIR}/*.cpp
    )
file(GLOB HEAD_FILES ${PROJECT_SOURCE_DIR}/test/*.h)

add_executable(${PROJECT_NAME} ${SRC_FILES} ${HEAD_FILES})

link_directories(${PROJECT_SOURCE_DIR}/../3rdparty/gperftools)
target_link_libraries(${PROJECT_NAME} gtest gtest_main tiny-ipc)
