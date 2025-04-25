#ifndef CC_MODEL_GOFUNCTION_H
#define CC_MODEL_GOFUNCTION_H

#include <vector>
#include <memory>

#include <odb/lazy-ptr.hxx>

#include "goentity.h"
#include "govariable.h"

namespace cc
{
namespace model
{

#pragma db object
struct GoFunction : GoEntity
{
  #pragma db on_delete(cascade)
  std::vector<odb::lazy_shared_ptr<GoVariable>> parameters;
  
  #pragma db on_delete(cascade)
  std::vector<odb::lazy_shared_ptr<GoVariable>> locals;
  
  #pragma db on_delete(cascade)
  std::vector<odb::lazy_shared_ptr<GoVariable>> results;

  std::string toString() const
  {
    return std::string("GoFunction")
      .append("\nid = ").append(std::to_string(id))
      .append("\nentityHash = ").append(std::to_string(entityHash))
      .append("\nname = ").append(name)
      .append("\nsignature = ").append(signature);
  }
};

typedef std::shared_ptr<GoFunction> GoFunctionPtr;

#pragma db view \
  object(GoFunction) object(GoVariable = Parameters : GoFunction::parameters)
struct GoFunctionParamCount
{
  #pragma db column("count(" + Parameters::id + ")")
  std::size_t count;
};

#pragma db view \
  object(GoFunction) object(GoVariable = Locals : GoFunction::locals)
struct GoFunctionLocalCount
{
  #pragma db column("count(" + Locals::id + ")")
  std::size_t count;
};

#pragma db view \
  object(GoFunction) object(GoVariable = Results : GoFunction::results)
struct GoFunctionResultCount
{
  #pragma db column("count(" + Results::id + ")")
  std::size_t count;
};

}
}

#endif