include_directories(
  include
  ${PROJECT_SOURCE_DIR}/model/include
  ${PROJECT_SOURCE_DIR}/util/include
  ${PROJECT_SOURCE_DIR}/parser/include)

add_library(goparser SHARED
  src/goparser.cpp)

target_compile_options(goparser PUBLIC -Wno-unknown-pragmas)

install(TARGETS goparser DESTINATION ${INSTALL_PARSER_DIR})
