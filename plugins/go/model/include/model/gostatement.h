#ifndef CC_MODEL_GOSTATEMENT_H
#define CC_MODEL_GOSTATEMENT_H

#include <memory>
#include <vector>

#include <odb/lazy-ptr.hxx>

#include "goentity.h"

namespace cc
{
namespace model
{

enum class GoStatementKind
{
  If,
  Else,
  For,
  Range,
  Switch,
  TypeSwitch,
  Case,
  Select,
  Defer,
  Go,
  Panic,
  Recover,
  Return,
  Assignment,
  Declaration,
  Other
};

inline std::string goStatementKindToString(GoStatementKind kind)
{
  switch (kind)
  {
    case GoStatementKind::If:         return "If";
    case GoStatementKind::Else:       return "Else";
    case GoStatementKind::For:        return "For";
    case GoStatementKind::Range:      return "Range";
    case GoStatementKind::Switch:     return "Switch";
    case GoStatementKind::TypeSwitch: return "TypeSwitch";
    case GoStatementKind::Case:       return "Case";
    case GoStatementKind::Select:     return "Select";
    case GoStatementKind::Defer:      return "Defer";
    case GoStatementKind::Go:         return "Go";
    case GoStatementKind::Panic:      return "Panic";
    case GoStatementKind::Recover:    return "Recover";
    case GoStatementKind::Return:     return "Return";
    case GoStatementKind::Assignment: return "Assignment";
    case GoStatementKind::Declaration:return "Declaration";
    case GoStatementKind::Other:      return "Other";
  }
  return "Unknown";
}

inline GoStatementKind goStatementKindFromString(const std::string& kindStr)
{
  if (kindStr == "If")          return GoStatementKind::If;
  if (kindStr == "Else")        return GoStatementKind::Else;
  if (kindStr == "For")         return GoStatementKind::For;
  if (kindStr == "Range")       return GoStatementKind::Range;
  if (kindStr == "Switch")      return GoStatementKind::Switch;
  if (kindStr == "TypeSwitch")  return GoStatementKind::TypeSwitch;
  if (kindStr == "Case")        return GoStatementKind::Case;
  if (kindStr == "Select")      return GoStatementKind::Select;
  if (kindStr == "Defer")       return GoStatementKind::Defer;
  if (kindStr == "Go")          return GoStatementKind::Go;
  if (kindStr == "Panic")       return GoStatementKind::Panic;
  if (kindStr == "Recover")     return GoStatementKind::Recover;
  if (kindStr == "Return")      return GoStatementKind::Return;
  if (kindStr == "Assignment")  return GoStatementKind::Assignment;
  if (kindStr == "Declaration") return GoStatementKind::Declaration;
  return GoStatementKind::Other;
}

#pragma db object
struct GoStatement : GoEntity
{
  GoStatementKind kind = GoStatementKind::Other;
  
  // Original node type from the AST
  std::string nodeType;
  
  // Statement-specific information
  std::string condition;   // For if, for, etc.
  std::string initialization; // For if, for, etc.
  std::string post;        // For for loops
  std::string collection;  // For range statements
  std::string key;         // For range statements
  std::string value;       // For range statements
  std::string call;        // For defer, go, etc.
  std::string argument;    // For panic
  
  // Case-specific fields
  bool isDefault = false;
  std::string caseValues;  // For switch cases
  std::string caseTypes;   // For type switch cases
  
  // Block structure
  #pragma db on_delete(cascade)
  std::vector<odb::lazy_shared_ptr<GoStatement>> children;
  
  int index = 0;   // Order within parent
  
  std::string toString() const
  {
    std::string stmtStr = std::string("GoStatement")
      .append("\nid = ").append(std::to_string(id))
      .append("\nentityHash = ").append(std::to_string(entityHash))
      .append("\nname = ").append(name)
      .append("\nkind = ").append(goStatementKindToString(kind))
      .append("\nnodeType = ").append(nodeType);
    
    // Add kind-specific details
    switch (kind)
    {
      case GoStatementKind::If:
        if (!condition.empty())
          stmtStr.append("\ncondition = ").append(condition);
        if (!initialization.empty())
          stmtStr.append("\ninitialization = ").append(initialization);
        break;
      
      case GoStatementKind::For:
        if (!initialization.empty())
          stmtStr.append("\ninitialization = ").append(initialization);
        if (!condition.empty())
          stmtStr.append("\ncondition = ").append(condition);
        if (!post.empty())
          stmtStr.append("\npost = ").append(post);
        break;
      
      case GoStatementKind::Range:
        if (!collection.empty())
          stmtStr.append("\ncollection = ").append(collection);
        if (!key.empty())
          stmtStr.append("\nkey = ").append(key);
        if (!value.empty())
          stmtStr.append("\nvalue = ").append(value);
        break;
      
      case GoStatementKind::Case:
        stmtStr.append("\nisDefault = ").append(isDefault ? "true" : "false");
        if (!caseValues.empty())
          stmtStr.append("\ncaseValues = ").append(caseValues);
        if (!caseTypes.empty())
          stmtStr.append("\ncaseTypes = ").append(caseTypes);
        break;
      
      case GoStatementKind::Defer:
      case GoStatementKind::Go:
        if (!call.empty())
          stmtStr.append("\ncall = ").append(call);
        break;
      
      case GoStatementKind::Panic:
        if (!argument.empty())
          stmtStr.append("\nargument = ").append(argument);
        break;
      
      default:
        break;
    }
    
    return stmtStr;
  }
};

typedef std::shared_ptr<GoStatement> GoStatementPtr;

}
}

#endif