#ifndef CC_MODEL_GOTYPE_H
#define CC_MODEL_GOTYPE_H

#include <memory>

#include "goentity.h"

namespace cc
{
namespace model
{

enum class GoTypeKind
{
  Array,
  Slice,
  Map,
  Channel,
  Pointer,
  Function,
  Basic,
  Other
};

inline std::string goTypeKindToString(GoTypeKind kind)
{
  switch (kind)
  {
    case GoTypeKind::Array:    return "Array";
    case GoTypeKind::Slice:    return "Slice";
    case GoTypeKind::Map:      return "Map";
    case GoTypeKind::Channel:  return "Channel";
    case GoTypeKind::Pointer:  return "Pointer";
    case GoTypeKind::Function: return "Function";
    case GoTypeKind::Basic:    return "Basic";
    case GoTypeKind::Other:    return "Other";
  }
  return "Unknown";
}

inline GoTypeKind goTypeKindFromString(const std::string& kindStr)
{
  if (kindStr == "Array")    return GoTypeKind::Array;
  if (kindStr == "Slice")    return GoTypeKind::Slice;
  if (kindStr == "Map")      return GoTypeKind::Map;
  if (kindStr == "Channel")  return GoTypeKind::Channel;
  if (kindStr == "Pointer")  return GoTypeKind::Pointer;
  if (kindStr == "Function") return GoTypeKind::Function;
  if (kindStr == "Basic")    return GoTypeKind::Basic;
  return GoTypeKind::Other;
}

#pragma db object
struct GoType : GoEntity
{
  GoTypeKind kind = GoTypeKind::Other;
  
  // Underlying type information
  std::string underlyingType;
  
  // Additional information based on kind
  std::string keyType;     // For maps
  std::string elementType; // For arrays, slices, channels
  std::string valueType;   // For maps
  std::string length;      // For arrays
  std::string direction;   // For channels (send, receive, bidirectional)
  
  std::string toString() const
  {
    std::string typeStr = std::string("GoType")
      .append("\nid = ").append(std::to_string(id))
      .append("\nentityHash = ").append(std::to_string(entityHash))
      .append("\nname = ").append(name)
      .append("\npackage = ").append(package)
      .append("\nkind = ").append(goTypeKindToString(kind));
    
    if (!underlyingType.empty())
      typeStr.append("\nunderlyingType = ").append(underlyingType);
    
    switch (kind)
    {
      case GoTypeKind::Map:
        typeStr.append("\nkeyType = ").append(keyType)
              .append("\nvalueType = ").append(valueType);
        break;
      
      case GoTypeKind::Array:
        typeStr.append("\nelementType = ").append(elementType)
              .append("\nlength = ").append(length);
        break;
      
      case GoTypeKind::Slice:
        typeStr.append("\nelementType = ").append(elementType);
        break;
      
      case GoTypeKind::Channel:
        typeStr.append("\nelementType = ").append(elementType)
              .append("\ndirection = ").append(direction);
        break;
      
      default:
        break;
    }
    
    return typeStr;
  }
};

typedef std::shared_ptr<GoType> GoTypePtr;

}
}

#endif