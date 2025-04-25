include_directories(
  include
  ${PROJECT_BINARY_DIR}/model/include
  ${PROJECT_SOURCE_DIR}/model/include
  ${PROJECT_SOURCE_DIR}/util/include
  ${PROJECT_SOURCE_DIR}/parser/include
  ${PLUGIN_DIR}/model/include)

add_library(goparser SHARED
  src/goparser.cpp)

target_link_libraries(goparser
  util
  model
  gomodel
  ${Boost_LIBRARIES})

target_compile_options(goparser PUBLIC -Wno-unknown-pragmas)

install(TARGETS goparser DESTINATION ${INSTALL_PARSER_DIR})