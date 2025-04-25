#ifndef CC_MODEL_GOINTERFACE_H
#define CC_MODEL_GOINTERFACE_H

#include <vector>
#include <memory>

#include <odb/lazy-ptr.hxx>

#include "goentity.h"

namespace cc
{
namespace model
{

typedef std::uint64_t GoInterfaceMethodId;

#pragma db object
struct GoInterfaceMethod
{
  #pragma db id auto
  GoInterfaceMethodId id;
  
  // Reference to parent interface
  #pragma db not_null
  std::uint64_t interfaceId;
  
  std::string name;
  std::string signature;
  int index = 0;
  int subIndex = 0;
  
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
    return std::string("GoInterfaceMethod")
      .append("\nid = ").append(std::to_string(id))
      .append("\ninterfaceId = ").append(std::to_string(interfaceId))
      .append("\nname = ").append(name)
      .append("\nsignature = ").append(signature);
  }
};

typedef std::shared_ptr<GoInterfaceMethod> GoInterfaceMethodPtr;

#pragma db object
struct GoInterface : GoEntity
{
  #pragma db on_delete(cascade)
  std::vector<odb::lazy_shared_ptr<GoInterfaceMethod>> methods;
  
  // Embedded interfaces
  std::vector<std::string> embeddedInterfaces;
  
  std::string toString() const
  {
    return std::string("GoInterface")
      .append("\nid = ").append(std::to_string(id))
      .append("\nentityHash = ").append(std::to_string(entityHash))
      .append("\nname = ").append(name)
      .append("\npackage = ").append(package);
  }
};

typedef std::shared_ptr<GoInterface> GoInterfacePtr;

#pragma db view \
  object(GoInterface) object(GoInterfaceMethod = Methods : GoInterface::methods)
struct GoInterfaceMethodCount
{
  #pragma db column("count(" + Methods::id + ")")
  std::size_t count;
};

}
}

#endif