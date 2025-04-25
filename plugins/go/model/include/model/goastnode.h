#ifndef CC_MODEL_GOASTNODE_H
#define CC_MODEL_GOASTNODE_H

#include <cstdint>
#include <string>
#include <vector>
#include <memory>

#include <odb/core.hxx>
#include <odb/lazy-ptr.hxx>
#include <odb/nullable.hxx>

#include <model/file.h>
#include <model/fileloc.h>

#include <util/hash.h>

namespace cc
{
namespace model
{

typedef std::uint64_t GoAstNodeId;

#pragma db object
struct GoAstNode
{
  enum class SymbolType
  {
    Package,
    Import,
    Function,
    Variable,
    Type,
    Reference,
    Other = 1000
  };

  enum class Visibility
  {
    Private,
    Public,
    Package
  };

  virtual ~GoAstNode() {}

  #pragma db id
  GoAstNodeId id = 0;

  std::string name;
  std::string nodeType;
  
  // Original JSON ID from parser
  std::string jsonId;
  
  // Parent node ID (if any)
  std::string parent;
  
  // Content is the raw text representation
  std::string content;

  // Signature of the node (for functions, etc.)
  std::string signature;
  
  // Package the node belongs to
  std::string package;

  // Position information
  #pragma db null
  FileLoc location;

  // Entity hash for linking related items
  std::uint64_t entityHash;

  SymbolType symbolType = SymbolType::Other;
  Visibility visibility = Visibility::Private;
  
  // Definition reference ID (if this is a reference)
  std::string defId;

  // Is the node exported
  bool exported = false;

  // Structured representation of the node
  std::string toString() const;

  bool operator< (const GoAstNode& other) const { return id <  other.id; }
  bool operator==(const GoAstNode& other) const { return id == other.id; }

#pragma db index("location_file_idx") member(location.file)
#pragma db index("entityHash_idx") member(entityHash)
};

typedef std::shared_ptr<GoAstNode> GoAstNodePtr;

inline std::string symbolTypeToString(GoAstNode::SymbolType type_)
{
  switch (type_)
  {
    case GoAstNode::SymbolType::Package:     return "Package";
    case GoAstNode::SymbolType::Import:      return "Import";
    case GoAstNode::SymbolType::Function:    return "Function";
    case GoAstNode::SymbolType::Variable:    return "Variable";
    case GoAstNode::SymbolType::Type:        return "Type";
    case GoAstNode::SymbolType::Reference:   return "Reference";
    case GoAstNode::SymbolType::Other:       return "Other";
  }

  return std::string();
}

inline GoAstNode::SymbolType symbolTypeFromString(const std::string& type_)
{
  if (type_ == "Package")   return GoAstNode::SymbolType::Package;
  if (type_ == "Import")    return GoAstNode::SymbolType::Import;
  if (type_ == "Function")  return GoAstNode::SymbolType::Function;
  if (type_ == "Variable")  return GoAstNode::SymbolType::Variable;
  if (type_ == "Type")      return GoAstNode::SymbolType::Type;
  if (type_ == "Reference") return GoAstNode::SymbolType::Reference;
  
  return GoAstNode::SymbolType::Other;
}

inline std::string visibilityToString(GoAstNode::Visibility visibility_)
{
  switch (visibility_)
  {
    case GoAstNode::Visibility::Public:    return "public";
    case GoAstNode::Visibility::Private:   return "private";
    case GoAstNode::Visibility::Package:   return "package";
  }

  return std::string();
}

inline GoAstNode::Visibility visibilityFromString(const std::string& visibility_)
{
  if (visibility_ == "public")   return GoAstNode::Visibility::Public;
  if (visibility_ == "private")  return GoAstNode::Visibility::Private;
  if (visibility_ == "package")  return GoAstNode::Visibility::Package;
  
  return GoAstNode::Visibility::Private;
}

inline std::string GoAstNode::toString() const
{
  return std::string("GoAstNode")
    .append("\nid = ").append(std::to_string(id))
    .append("\nname = ").append(name)
    .append("\njsonId = ").append(jsonId)
    .append("\nparent = ").append(parent)
    .append("\ncontent = ").append(content)
    .append("\nsignature = ").append(signature)
    .append("\npackage = ").append(package)
    .append("\nlocation = ").append(location.file ? location.file->path : "")
    .append("\nsymbolType = ").append(symbolTypeToString(symbolType))
    .append("\nvisibility = ").append(visibilityToString(visibility))
    .append("\nexported = ").append(exported ? "true" : "false");
}

inline std::uint64_t createIdentifier(const GoAstNode& astNode_)
{
  std::string res;

  res
    .append(astNode_.jsonId).append(":")
    .append(astNode_.name).append(":")
    .append(astNode_.package).append(":")
    .append(symbolTypeToString(astNode_.symbolType)).append(":");

  if (astNode_.location.file)
    res
      .append(std::to_string(
        astNode_.location.file->id)).append(":")
      .append(std::to_string(
        astNode_.location.range.start.line)).append(":")
      .append(std::to_string(
        astNode_.location.range.start.column)).append(":")
      .append(std::to_string(
        astNode_.location.range.end.line)).append(":")
      .append(std::to_string(
        astNode_.location.range.end.column));
  else
    res.append("null");

  return util::fnvHash(res);
}

#pragma db view object(GoAstNode)
struct GoAstNodeIds
{
  GoAstNodeId id;
};

#pragma db view \
  object(GoAstNode) object(File = LocFile : GoAstNode::location.file) \
  query ((?) + "GROUP BY" + LocFile::id + "ORDER BY" + LocFile::id)
struct AstCountGroupByFiles
{
  #pragma db column(LocFile::id)
  FileId file;

  #pragma db column("count(" + GoAstNode::id + ")")
  std::size_t count;
};

#pragma db view object(GoAstNode)
struct GoAstCount
{
  #pragma db column("count(" + GoAstNode::id + ")")
  std::size_t count;
};

}
}

#endif