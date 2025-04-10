include_directories(
  include
  ${CMAKE_CURRENT_BINARY_DIR}/gen-cpp
  ${PROJECT_SOURCE_DIR}/util/include
  ${PROJECT_SOURCE_DIR}/webserver/include)

include_directories(SYSTEM
  ${THRIFT_LIBTHRIFT_INCLUDE_DIRS})

add_custom_command(
  OUTPUT
    ${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/GoService.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/GoService.h
    ${CMAKE_CURRENT_BINARY_DIR}/gen-cpp
  COMMAND
    ${THRIFT_EXECUTABLE} --gen cpp
      -o ${CMAKE_CURRENT_BINARY_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/go.thrift
  DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/go.thrift
  COMMENT
    "Generating Thrift for go.thrift")

add_library(gothrift STATIC
  ${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/GoService.cpp)

target_compile_options(gothrift PUBLIC -fPIC)

add_library(goservice SHARED
  src/goservice.cpp
  src/plugin.cpp)

target_compile_options(goservice PUBLIC -Wno-unknown-pragmas)

target_link_libraries(goservice
  util
  ${THRIFT_LIBTHRIFT_LIBRARIES}
  ${ODB_LIBRARIES}
  gothrift)

install(TARGETS goservice DESTINATION ${INSTALL_SERVICE_DIR})
