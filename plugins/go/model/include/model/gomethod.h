#ifndef CC_MODEL_GOMETHOD_H
#define CC_MODEL_GOMETHOD_H

#include <memory>

#include "gofunction.h"

namespace cc
{
namespace model
{

#pragma db object
struct GoMethod : GoFunction
{
  // Receiver type information
  std::string receiverType;
  std::string receiverName;
  bool isPointerReceiver = false;
  
  std::string toString() const
  {
    return std::string("GoMethod")
      .append("\nid = ").append(std::to_string(id))
      .append("\nentityHash = ").append(std::to_string(entityHash))
      .append("\nname = ").append(name)
      .append("\nsignature = ").append(signature)
      .append("\nreceiverType = ").append(receiverType)
      .append("\nreceiverName = ").append(receiverName)
      .append("\nisPointerReceiver = ").append(isPointerReceiver ? "true" : "false");
  }
};

typedef std::shared_ptr<GoMethod> GoMethodPtr;

}
}

#endif