#ifndef CC_MODEL_GOIMPORT_H
#define CC_MODEL_GOIMPORT_H

#include "goentity.h"

namespace cc
{
namespace model
{

#pragma db object
struct GoImport : GoEntity
{
  std::string path;     // Import path

  std::string toString() const
  {
    return std::string("GoImport")
      .append("\nid = ").append(std::to_string(id))
      .append("\nentityHash = ").append(std::to_string(entityHash))
      .append("\nname = ").append(name)
      .append("\npath = ").append(path);
  }
};

typedef std::shared_ptr<GoImport> GoImportPtr;

}
}

#endif