#ifndef CC_MODEL_GOVARIABLE_H
#define CC_MODEL_GOVARIABLE_H

#include "goentity.h"

namespace cc
{
namespace model
{

#pragma db object
struct GoVariable : GoEntity
{
  std::string type;          // Type of the variable
  std::string parentFunc;    // ID of parent function if any
  
  // Extra info from the JSON
  bool isParameter = false;
  bool isResult = false;
  int paramIndex = -1;
  int resultIndex = -1;

  std::string toString() const
  {
    return std::string("GoVariable")
      .append("\nid = ").append(std::to_string(id))
      .append("\nentityHash = ").append(std::to_string(entityHash))
      .append("\nname = ").append(name)
      .append("\ntype = ").append(type)
      .append("\nparentFunc = ").append(parentFunc)
      .append("\nisParameter = ").append(isParameter ? "true" : "false")
      .append("\nisResult = ").append(isResult ? "true" : "false");
  }
};

typedef std::shared_ptr<GoVariable> GoVariablePtr;

}
}

#endif