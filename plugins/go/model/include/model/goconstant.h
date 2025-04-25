#ifndef CC_MODEL_GOCONSTANT_H
#define CC_MODEL_GOCONSTANT_H

#include <memory>

#include "goentity.h"

namespace cc
{
namespace model
{

#pragma db object
struct GoConstant : GoEntity
{
  // Constant value
  std::string value;
  
  // Type of the constant
  std::string type;
  
  // Position in const block
  int index = 0;
  
  std::string toString() const
  {
    return std::string("GoConstant")
      .append("\nid = ").append(std::to_string(id))
      .append("\nentityHash = ").append(std::to_string(entityHash))
      .append("\nname = ").append(name)
      .append("\ntype = ").append(type)
      .append("\nvalue = ").append(value)
      .append("\nexported = ").append(exported ? "true" : "false");
  }
};

typedef std::shared_ptr<GoConstant> GoConstantPtr;

}
}

#endif