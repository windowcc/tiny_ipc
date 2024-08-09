
set(${PROJECT_NAME}_source_files
    Buffer.cpp
    Ipc.cpp
    Handle.cpp
    Descriptor.cpp
)

set(${PROJECT_NAME}_sync_source_files
    sync/Condition.cpp
    sync/Mutex.cpp
    sync/Waiter.cpp
)

file(GLOB HEAD_FILES
  ${PROJECT_SOURCE_DIR}/include/ipc/*.h
  ${PROJECT_SOURCE_DIR}/circ/*.h
  ${PROJECT_SOURCE_DIR}/memory/*.h
  ${PROJECT_SOURCE_DIR}/platform/*.h
  ${PROJECT_SOURCE_DIR}/utility/*.h)

if (LIBIPC_BUILD_SHARED_LIBS)
  add_library(${PROJECT_NAME} SHARED 
    ${${PROJECT_NAME}_source_files}
    ${${PROJECT_NAME}_sync_source_files}
    ${${PROJECT_NAME}_platform_source_files}
    ${HEAD_FILES}
  )
  target_compile_definitions(${PROJECT_NAME}
    INTERFACE
      LIBIPC_LIBRARY_SHARED_USING__
    PRIVATE
      LIBIPC_LIBRARY_SHARED_BUILDING__)
else()
  add_library(${PROJECT_NAME} STATIC 
    ${${PROJECT_NAME}_source_files}
    ${${PROJECT_NAME}_sync_source_files}
    ${HEAD_FILES}
  )
endif()

# set (PACKAGE_VERSION 1.3.0)
# set output directory
set_target_properties(${PROJECT_NAME}
	PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# set version
# set_target_properties(${PROJECT_NAME}
#     PROPERTIES
#     VERSION ${PACKAGE_VERSION}
#     SOVERSION 3)

target_include_directories(${PROJECT_NAME}
  PUBLIC  
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
  PRIVATE ${PROJECT_SOURCE_DIR}
          $<$<BOOL:UNIX>:${PROJECT_SOURCE_DIR}/platform/linux>)

if(NOT MSVC)
  target_link_libraries(${PROJECT_NAME} PUBLIC
    $<$<NOT:$<STREQUAL:${CMAKE_SYSTEM_NAME},QNX>>:pthread>
    $<$<NOT:$<OR:$<STREQUAL:${CMAKE_SYSTEM_NAME},Windows>,$<STREQUAL:${CMAKE_SYSTEM_NAME},QNX>>>:rt>)
endif()

install(
  TARGETS ${PROJECT_NAME}
  EXPORT tiny-ipc-targets
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

install(EXPORT tiny-ipc-targets
	FILE tiny-ipc-targets.cmake
	NAMESPACE tiny-ipc::
	DESTINATION share/tiny-ipc
)

# file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/tiny-ipc-config.cmake.in"
# [[include(CMakeFindDependencyMacro)
# include("${CMAKE_CURRENT_LIST_DIR}/tiny-ipc-targets.cmake")
# ]])
# configure_file("${CMAKE_CURRENT_BINARY_DIR}/tiny-ipc-config.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/tiny-ipc-config.cmake" @ONLY)
# install(FILES ${CMAKE_CURRENT_BINARY_DIR}/tiny-ipc-config.cmake DESTINATION share/tiny-ipc)

# include(CMakePackageConfigHelpers)
# write_basic_package_version_file(
# 	cppIpcConfigVersion.cmake
# 	VERSION ${PACKAGE_VERSION}
# 	COMPATIBILITY AnyNewerVersion
# )
# install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cppIpcConfigVersion.cmake DESTINATION share/tiny-ipc)