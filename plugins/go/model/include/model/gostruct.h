#ifndef CC_MODEL_GOSTRUCT_H
#define CC_MODEL_GOSTRUCT_H

#include <vector>
#include <map>
#include <memory>

#include <odb/lazy-ptr.hxx>

#include "goentity.h"

namespace cc
{
namespace model
{

typedef std::uint64_t GoStructFieldId;

#pragma db object
struct GoStructField
{
  #pragma db id auto
  GoStructFieldId id;
  
  // Reference to parent struct
  #pragma db not_null
  std::uint64_t structId;
  
  std::string name;
  std::string type;
  bool exported = false;
  bool embedded = false;
  int index = 0;
  int subIndex = 0;
  
  // Struct tags (json, xml, etc.)
  std::string tags;
  
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
    return std::string("GoStructField")
      .append("\nid = ").append(std::to_string(id))
      .append("\nstructId = ").append(std::to_string(structId))
      .append("\nname = ").append(name)
      .append("\ntype = ").append(type)
      .append("\nexported = ").append(exported ? "true" : "false")
      .append("\nembedded = ").append(embedded ? "true" : "false");
  }
};

typedef std::shared_ptr<GoStructField> GoStructFieldPtr;

#pragma db object
struct GoStruct : GoEntity
{
  #pragma db on_delete(cascade)
  std::vector<odb::lazy_shared_ptr<GoStructField>> fields;
  
  // Count of fields
  int fieldCount = fields.size();
  
  // Embedded types
  std::vector<std::string> embeddedTypes;
  
  std::string toString() const
  {
    return std::string("GoStruct")
      .append("\nid = ").append(std::to_string(id))
      .append("\nentityHash = ").append(std::to_string(entityHash))
      .append("\nname = ").append(name)
      .append("\npackage = ").append(package)
      .append("\nfieldCount = ").append(std::to_string(fieldCount));
  }
};

typedef std::shared_ptr<GoStruct> GoStructPtr;

}
}

#endif