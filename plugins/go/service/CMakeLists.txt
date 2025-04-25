include_directories(
  include
  ${PROJECT_SOURCE_DIR}/util/include
  ${PROJECT_SOURCE_DIR}/webserver/include
  ${PROJECT_SOURCE_DIR}/model/include
  ${PROJECT_BINARY_DIR}/service/language/gen-cpp
  ${PROJECT_BINARY_DIR}/service/project/gen-cpp
  ${PROJECT_SOURCE_DIR}/service/project/include
  ${PLUGIN_DIR}/model/include)

include_directories(SYSTEM
  ${THRIFT_LIBTHRIFT_INCLUDE_DIRS})

add_library(goservice SHARED
  src/goservice.cpp
  src/diagram.cpp
  src/plugin.cpp)

target_compile_options(goservice PUBLIC -Wno-unknown-pragmas)

target_link_libraries(goservice
  util
  model
  gomodel
  mongoose
  projectservice
  languagethrift
  gvc
  ${THRIFT_LIBTHRIFT_LIBRARIES})

install(TARGETS goservice DESTINATION ${INSTALL_SERVICE_DIR})