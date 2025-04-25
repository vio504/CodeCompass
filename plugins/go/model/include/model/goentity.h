#ifndef CC_MODEL_GOENTITY_H
#define CC_MODEL_GOENTITY_H

#include <string>
#include <set>

#include <odb/nullable.hxx>

#include "goastnode.h"

namespace cc
{
namespace model
{

typedef std::uint64_t GoEntityId;

#pragma db object polymorphic
struct GoEntity
{
  virtual ~GoEntity() {}

  #pragma db id auto
  GoEntityId id;

  #pragma db unique
  GoAstNodeId astNodeId;

  std::uint64_t entityHash = 0;

  std::string name;
  std::string signature;
  std::string package;

  std::string jsonId;
  
  // Additional fields from the JSON format
  std::string structName;
  std::string docText;
  bool exported = false;
  std::string visibility;
  std::string scope;

#pragma db index member(entityHash)
};

typedef std::shared_ptr<GoEntity> GoEntityPtr;

}
}

#endif