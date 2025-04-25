set(ODB_SOURCES
  include/model/goastnode.h
  include/model/goentity.h
  include/model/gofunction.h
  include/model/govariable.h
  include/model/gopackage.h
  include/model/goimport.h
  include/model/gostruct.h
  include/model/gointerface.h
  include/model/goenum.h
  include/model/gotype.h
  include/model/gostatement.h
  include/model/gomethod.h
  include/model/goconstant.h)

generate_odb_files("${ODB_SOURCES}")

add_odb_library(gomodel ${ODB_CXX_SOURCES})
target_link_libraries(gomodel model)

install_sql()