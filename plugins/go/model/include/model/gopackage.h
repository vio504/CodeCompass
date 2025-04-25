#ifndef CC_MODEL_GOPACKAGE_H
#define CC_MODEL_GOPACKAGE_H

#include "goentity.h"

namespace cc
{
namespace model
{

#pragma db object
struct GoPackage : GoEntity
{
  std::string toString() const
  {
    return std::string("GoPackage")
      .append("\nid = ").append(std::to_string(id))
      .append("\nentityHash = ").append(std::to_string(entityHash))
      .append("\nname = ").append(name);
  }
};

typedef std::shared_ptr<GoPackage> GoPackagePtr;

}
}

#endif