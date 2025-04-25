#ifndef CC_MODEL_GOENUM_H
#define CC_MODEL_GOENUM_H

#include <vector>
#include <memory>

#include <odb/lazy-ptr.hxx>

#include "goentity.h"

namespace cc
{
namespace model
{

typedef std::uint64_t GoEnumConstantId;

#pragma db object
struct GoEnumConstant
{
  #pragma db id auto
  GoEnumConstantId id;
  
  // Reference to parent enum
  #pragma db not_null
  std::uint64_t enumId;
  
  std::string name;
  std::string value;
  bool exported = false;
  int index = 0;
  
  std::string jsonId;    // Original JSON ID
  std::string docText;   // Documentation
  
  // Position information
  std::string file;      // File path
  int line = 0;
  int column = 0;
  int endLine = 0;
  int endColumn = 0;
  
  std::string toString() const
  {
    return std::string("GoEnumConstant")
      .append("\nid = ").append(std::to_string(id))
      .append("\nenumId = ").append(std::to_string(enumId))
      .append("\nname = ").append(name)
      .append("\nvalue = ").append(value)
      .append("\nexported = ").append(exported ? "true" : "false");
  }
};

typedef std::shared_ptr<GoEnumConstant> GoEnumConstantPtr;

#pragma db object
struct GoEnum : GoEntity
{
  #pragma db on_delete(cascade)
  std::vector<odb::lazy_shared_ptr<GoEnumConstant>> constants;
  
  // Type of the enum (always int in Go, but could be other type aliases)
  std::string type = "int";
  
  std::string toString() const
  {
    return std::string("GoEnum")
      .append("\nid = ").append(std::to_string(id))
      .append("\nentityHash = ").append(std::to_string(entityHash))
      .append("\nname = ").append(name)
      .append("\npackage = ").append(package)
      .append("\ntype = ").append(type);
  }
};

typedef std::shared_ptr<GoEnum> GoEnumPtr;

#pragma db view \
  object(GoEnum) object(GoEnumConstant = Constants : GoEnum::constants)
struct GoEnumConstantCount
{
  #pragma db column("count(" + Constants::id + ")")
  std::size_t count;
};

}
}

#endif