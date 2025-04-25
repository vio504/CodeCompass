#include <algorithm>
#include <queue>
#include <regex>

#include <util/util.h>
#include <util/logutil.h>

#include <service/goservice.h>
#include "diagram.h"

namespace cc
{
  namespace service
  {
    namespace language
    {

      typedef odb::query<model::GoAstNode> AstQuery;
      typedef odb::result<model::GoAstNode> AstResult;
      typedef odb::query<model::GoFunction> FuncQuery;
      typedef odb::result<model::GoFunction> FuncResult;
      typedef odb::query<model::GoMethod> MethodQuery;
      typedef odb::result<model::GoMethod> MethodResult;
      typedef odb::query<model::GoStruct> StructQuery;
      typedef odb::result<model::GoStruct> StructResult;
      typedef odb::query<model::GoStructField> FieldQuery;
      typedef odb::result<model::GoStructField> FieldResult;
      typedef odb::query<model::GoInterface> InterfaceQuery;
      typedef odb::result<model::GoInterface> InterfaceResult;
      typedef odb::query<model::GoEnum> EnumQuery;
      typedef odb::result<model::GoEnum> EnumResult;
      typedef odb::query<model::GoVariable> VarQuery;
      typedef odb::result<model::GoVariable> VarResult;
      typedef odb::query<model::GoConstant> ConstQuery;
      typedef odb::result<model::GoConstant> ConstResult;
      typedef odb::query<model::GoType> TypeQuery;
      typedef odb::result<model::GoType> TypeResult;
      typedef odb::query<model::GoStatement> StmtQuery;
      typedef odb::result<model::GoStatement> StmtResult;
      typedef odb::query<model::File> FileQuery;
      typedef odb::result<model::File> FileResult;
      typedef odb::query<model::GoPackage> PackageQuery;
      typedef odb::result<model::GoPackage> PackageResult;
      typedef odb::query<model::GoImport> ImportQuery;
      typedef odb::result<model::GoImport> ImportResult;

      // transforms a model::GoAstNode to an AstNodeInfo Thrift object

      struct CreateAstNodeInfo
      {
        typedef std::map<model::GoAstNodeId, std::vector<std::string>> TagMap;

        CreateAstNodeInfo(const TagMap &tags_ = {}) : _tags(tags_)
        {
        }

        // Returns the Thrift object for this Go AST node.

        cc::service::language::AstNodeInfo operator()(
            const model::GoAstNode &astNode_)
        {
          cc::service::language::AstNodeInfo ret;

          ret.__set_id(std::to_string(astNode_.id));
          ret.__set_entityHash(astNode_.entityHash);
          ret.__set_astNodeType(astNode_.nodeType);
          ret.__set_symbolType(model::symbolTypeToString(astNode_.symbolType));
          ret.__set_astNodeValue(astNode_.name);

          ret.range.range.startpos.line = astNode_.location.range.start.line;
          ret.range.range.startpos.column = astNode_.location.range.start.column;
          ret.range.range.endpos.line = astNode_.location.range.end.line;
          ret.range.range.endpos.column = astNode_.location.range.end.column;

          if (astNode_.location.file)
            ret.range.file = std::to_string(astNode_.location.file.object_id());

          TagMap::const_iterator it = _tags.find(astNode_.id);
          if (it != _tags.end())
            ret.__set_tags(it->second);

          return ret;
        }

        const std::map<model::GoAstNodeId, std::vector<std::string>> &_tags;
      };

      GoServiceHandler::GoServiceHandler(
          std::shared_ptr<odb::database> db_,
          std::shared_ptr<std::string> datadir_,
          const cc::webserver::ServerContext &context_)
          : _db(db_),
            _transaction(db_),
            _datadir(datadir_),
            _context(context_)
      {
      }
      void GoServiceHandler::getGoString(std::string &str_)
      {
        str_ = _context.options["go-result"].as<std::string>();
      }

      void GoServiceHandler::getFileTypes(std::vector<std::string> &_return)
      {
        _return.push_back("GO");
        _return.push_back("Dir");
      }

      void GoServiceHandler::getAstNodeInfo(
          AstNodeInfo &_return,
          const core::AstNodeId &astNodeId_)
      {
        _return = _transaction([this, &astNodeId_]()
                               { return CreateAstNodeInfo()(queryGoAstNode(astNodeId_)); });
      }

      void GoServiceHandler::getAstNodeInfoByPosition(
          AstNodeInfo &_return,
          const core::FilePosition &fpos_)
      {
        _transaction([&, this]()
                     {
    //--- Query nodes at the given position ---//

    AstResult nodes = _db->query<model::GoAstNode>(
      AstQuery::location.file == std::stoull(fpos_.file) &&
      // StartPos <= Pos
      ((AstQuery::location.range.start.line == fpos_.pos.line &&
        AstQuery::location.range.start.column <= fpos_.pos.column) ||
       AstQuery::location.range.start.line < fpos_.pos.line) &&
      // Pos < EndPos
      ((AstQuery::location.range.end.line == fpos_.pos.line &&
        AstQuery::location.range.end.column > fpos_.pos.column) ||
       AstQuery::location.range.end.line > fpos_.pos.line));

    //--- Select innermost clickable node ---//

    model::Range minRange(model::Position(0, 0), model::Position());
    model::GoAstNode min;

    for (const model::GoAstNode& node : nodes)
    {
      if (node.location.range < minRange)
      {
        min = node;
        minRange = node.location.range;
      }
    }

    _return = _transaction([this, &min](){
      return CreateAstNodeInfo(getTags({min}))(min);
    }); });
      }

      void GoServiceHandler::getSourceText(
          std::string &_return,
          const core::AstNodeId &astNodeId_)
      {
        _return = _transaction([this, &astNodeId_]()
                               {
    model::GoAstNode astNode = queryGoAstNode(astNodeId_);

    if (astNode.location.file)
      return cc::util::textRange(
        astNode.location.file.load()->content.load()->content,
        astNode.location.range.start.line,
        astNode.location.range.start.column,
        astNode.location.range.end.line,
        astNode.location.range.end.column);

    return std::string(); });
      }
      void GoServiceHandler::getDocumentation(std::string& return_, const core::AstNodeId& astNodeId_)
      {
        return_ = _transaction([&, this](){
          model::GoAstNode node = queryGoAstNode(astNodeId_);
          
          // Try to find the corresponding entity for this AST node
          std::string documentation;
          
          // Check if this node has an associated entity and get its documentation
          switch (node.symbolType)
          {
            case model::GoAstNode::SymbolType::Function:
            {
              FuncResult funcs = _db->query<model::GoFunction>(
                FuncQuery::astNodeId == node.id);
              if (!funcs.empty())
              {
                const auto& func = *funcs.begin();
                if (!func.docText.empty())
                  documentation = func.docText;
              }
              break;
            }
            
            case model::GoAstNode::SymbolType::Type:
            {
              // Check if it's a struct, interface, or type alias
              StructResult structs = _db->query<model::GoStruct>(
                StructQuery::astNodeId == node.id);
              if (!structs.empty())
              {
                const auto& struct_ = *structs.begin();
                if (!struct_.docText.empty())
                  documentation = struct_.docText;
                  
                // Add field documentation
                for (const auto& field : struct_.fields)
                {
                  auto loadedField = field.load();
                  documentation += "<div class=\"group\"><div class=\"signature\">";
                  documentation += std::string("<span class=\"") + 
                    (loadedField->exported ? "icon-public" : "icon-private") + 
                    "\"></span>";
                  documentation += loadedField->name + " " + loadedField->type;
                  documentation += "</div>";
                  
                  if (!loadedField->docText.empty())
                    documentation += loadedField->docText;
                    
                  documentation += "</div>";
                }
                break;
              }
              
              // Try interface
              InterfaceResult interfaces = _db->query<model::GoInterface>(
                InterfaceQuery::astNodeId == node.id);
              if (!interfaces.empty())
              {
                const auto& interface_ = *interfaces.begin();
                if (!interface_.docText.empty())
                  documentation = interface_.docText;
                  
                // Add method documentation
                for (const auto& method : interface_.methods)
                {
                  auto loadedMethod = method.load();
                  documentation += "<div class=\"group\"><div class=\"signature\">";
                  documentation += loadedMethod->name + " " + loadedMethod->signature;
                  documentation += "</div>";
                  
                  if (!loadedMethod->docText.empty())
                    documentation += loadedMethod->docText;
                    
                  documentation += "</div>";
                }
                break;
              }
              
              // Try Go type
              TypeResult types = _db->query<model::GoType>(
                TypeQuery::astNodeId == node.id);
              if (!types.empty())
              {
                const auto& type = *types.begin();
                if (!type.docText.empty())
                  documentation = type.docText;
              }
              break;
            }
            
            case model::GoAstNode::SymbolType::Variable:
            {
              VarResult vars = _db->query<model::GoVariable>(
                VarQuery::astNodeId == node.id);
              if (!vars.empty())
              {
                const auto& var = *vars.begin();
                if (!var.docText.empty())
                  documentation = var.docText;
              }
              break;
            }
            
            case model::GoAstNode::SymbolType::Package:
            {
              PackageResult packages = _db->query<model::GoPackage>(
                PackageQuery::astNodeId == node.id);
              if (!packages.empty())
              {
                const auto& pkg = *packages.begin();
                if (!pkg.docText.empty())
                  documentation = pkg.docText;
              }
              break;
            }
            
            case model::GoAstNode::SymbolType::Import:
            {
              ImportResult imports = _db->query<model::GoImport>(
                ImportQuery::astNodeId == node.id);
              if (!imports.empty())
              {
                const auto& import = *imports.begin();
                if (!import.docText.empty())
                  documentation = import.docText;
              }
              break;
            }
            
            default:
              break;
          }
          
          // Format the documentation
          if (!documentation.empty())
          {
            return "<div class=\"main-doc\">" + documentation + "</div>";
          }
          
          // If no documentation found, return empty string
          return std::string();
        });
      }
      
    
      

      void GoServiceHandler::getProperties(
        std::map<std::string, std::string>& return_,
        const core::AstNodeId& astNodeId_)
      {
        _transaction([&, this](){
          model::GoAstNode node = queryGoAstNode(astNodeId_);
      
          // Common properties for all nodes
          return_["AST Node ID"] = std::to_string(node.id);
          return_["Entity Hash"] = std::to_string(node.entityHash);
          return_["Name"] = node.name;
          return_["Symbol Type"] = model::symbolTypeToString(node.symbolType);
          return_["Node Type"] = node.nodeType;
          
          if (!node.package.empty())
            return_["Package"] = node.package;
            
          if (node.exported)
            return_["Exported"] = "true";
            
          if (!node.signature.empty())
            return_["Signature"] = node.signature;
            
          // Display visibility if it's not private (default)
          if (node.visibility != model::GoAstNode::Visibility::Private)
            return_["Visibility"] = model::visibilityToString(node.visibility);
      
          // Type-specific properties
          switch (node.symbolType)
          {
            case model::GoAstNode::SymbolType::Variable:
            {
              VarResult variables = _db->query<model::GoVariable>(
                VarQuery::entityHash == node.entityHash);
      
              if (!variables.empty())
              {
                model::GoVariable variable = *variables.begin();
                
                if (!variable.type.empty())
                  return_["Type"] = variable.type;
                  
                if (!variable.docText.empty())
                  return_["Documentation"] = variable.docText;
                  
                if (variable.isParameter)
                  return_["Parameter"] = "true";
                  
                if (variable.isResult)
                  return_["Result"] = "true";
                  
                if (variable.paramIndex >= 0)
                  return_["Parameter Index"] = std::to_string(variable.paramIndex);
                  
                if (variable.resultIndex >= 0)
                  return_["Result Index"] = std::to_string(variable.resultIndex);
                  
                if (!variable.parentFunc.empty())
                  return_["Parent Function"] = variable.parentFunc;
                  
                if (!variable.scope.empty())
                  return_["Scope"] = variable.scope;
              }
              break;
            }
      
            case model::GoAstNode::SymbolType::Function:
            {
              // Check for regular function
              FuncResult functions = _db->query<model::GoFunction>(
                FuncQuery::entityHash == node.entityHash);
      
              if (!functions.empty())
              {
                model::GoFunction function = *functions.begin();
                
                if (!function.docText.empty())
                  return_["Documentation"] = function.docText;
                  
                // Count parameters and results
                if (!function.parameters.empty())
                  return_["Parameter Count"] = std::to_string(function.parameters.size());
                  
                if (!function.results.empty())
                  return_["Result Count"] = std::to_string(function.results.size());
                  
                if (!function.locals.empty())
                  return_["Local Variables"] = std::to_string(function.locals.size());
                  
                if (!function.scope.empty())
                  return_["Scope"] = function.scope;
              }
              
              // Check if it's a method
              MethodResult methods = _db->query<model::GoMethod>(
                MethodQuery::entityHash == node.entityHash);
                
              if (!methods.empty())
              {
                model::GoMethod method = *methods.begin();
                
                if (!method.receiverType.empty())
                  return_["Receiver Type"] = method.receiverType;
                  
                if (!method.receiverName.empty())
                  return_["Receiver Name"] = method.receiverName;
                  
                if (method.isPointerReceiver)
                  return_["Pointer Receiver"] = "true";
              }
              break;
            }
      
            case model::GoAstNode::SymbolType::Type:
            {
              bool foundType = false;
              
              // Check for struct
              StructResult structs = _db->query<model::GoStruct>(
                StructQuery::entityHash == node.entityHash);
      
              if (!structs.empty())
              {
                foundType = true;
                model::GoStruct struct_ = *structs.begin();
                
                return_["Type Kind"] = "Struct";
                
                if (!struct_.docText.empty())
                  return_["Documentation"] = struct_.docText;
                
                //return_["Field Count"] = std::to_string(struct_.fieldCount);
                
                if (!struct_.embeddedTypes.empty()) {
                  std::string embedded;
                  for (size_t i = 0; i < struct_.embeddedTypes.size(); ++i) {
                    if (i > 0) embedded += ", ";
                    embedded += struct_.embeddedTypes[i];
                  }
                  return_["Embedded Types"] = embedded;
                }
              }
              
              // Check for interface
              if (!foundType) {
                InterfaceResult interfaces = _db->query<model::GoInterface>(
                  InterfaceQuery::entityHash == node.entityHash);
                
                if (!interfaces.empty()) {
                  foundType = true;
                  model::GoInterface interface_ = *interfaces.begin();
                  
                  return_["Type Kind"] = "Interface";
                  
                  if (!interface_.docText.empty())
                    return_["Documentation"] = interface_.docText;
                    
                  if (!interface_.methods.empty())
                    return_["Method Count"] = std::to_string(interface_.methods.size());
                    
                  if (!interface_.embeddedInterfaces.empty()) {
                    std::string embedded;
                    for (size_t i = 0; i < interface_.embeddedInterfaces.size(); ++i) {
                      if (i > 0) embedded += ", ";
                      embedded += interface_.embeddedInterfaces[i];
                    }
                    return_["Embedded Interfaces"] = embedded;
                  }
                }
              }
              
              // Check for enum
              if (!foundType) {
                EnumResult enums = _db->query<model::GoEnum>(
                  EnumQuery::entityHash == node.entityHash);
                  
                if (!enums.empty()) {
                  foundType = true;
                  model::GoEnum enum_ = *enums.begin();
                  
                  return_["Type Kind"] = "Enum";
                  
                  if (!enum_.docText.empty())
                    return_["Documentation"] = enum_.docText;
                    
                  if (!enum_.constants.empty())
                    return_["Constant Count"] = std::to_string(enum_.constants.size());
                    
                  if (!enum_.type.empty())
                    return_["Underlying Type"] = enum_.type;
                }
              }
              
              // Check for other types
              if (!foundType) {
                TypeResult types = _db->query<model::GoType>(
                  TypeQuery::entityHash == node.entityHash);
                  
                if (!types.empty()) {
                  model::GoType type = *types.begin();
                  
                  return_["Type Kind"] = goTypeKindToString(type.kind);
                  
                  if (!type.underlyingType.empty())
                    return_["Underlying Type"] = type.underlyingType;
                    
                  // Type-specific properties
                  switch (type.kind) {
                    case model::GoTypeKind::Array:
                      if (!type.elementType.empty())
                        return_["Element Type"] = type.elementType;
                      if (!type.length.empty())
                        return_["Length"] = type.length;
                      break;
                      
                    case model::GoTypeKind::Slice:
                      if (!type.elementType.empty())
                        return_["Element Type"] = type.elementType;
                      break;
                      
                    case model::GoTypeKind::Map:
                      if (!type.keyType.empty())
                        return_["Key Type"] = type.keyType;
                      if (!type.valueType.empty())
                        return_["Value Type"] = type.valueType;
                      break;
                      
                    case model::GoTypeKind::Channel:
                      if (!type.elementType.empty())
                        return_["Element Type"] = type.elementType;
                      if (!type.direction.empty())
                        return_["Direction"] = type.direction;
                      break;
                      
                    case model::GoTypeKind::Pointer:
                      if (!type.elementType.empty())
                        return_["Pointed Type"] = type.elementType;
                      break;
                      
                    case model::GoTypeKind::Function:
                      // Function type properties would go here
                      break;
                      
                    default:
                      break;
                  }
                }
              }
              break;
            }
            
            case model::GoAstNode::SymbolType::Package:
            {
              PackageResult packages = _db->query<model::GoPackage>(
                PackageQuery::entityHash == node.entityHash);
                
              if (!packages.empty())
              {
                model::GoPackage package = *packages.begin();
                
                if (!package.docText.empty())
                  return_["Documentation"] = package.docText;
                  
                if (!package.scope.empty())
                  return_["Scope"] = package.scope;
              }
              break;
            }
            
            case model::GoAstNode::SymbolType::Import:
            {
              ImportResult imports = _db->query<model::GoImport>(
                ImportQuery::entityHash == node.entityHash);
                
              if (!imports.empty())
              {
                model::GoImport import = *imports.begin();
                
                if (!import.path.empty())
                  return_["Path"] = import.path;
                  
                if (!import.docText.empty())
                  return_["Documentation"] = import.docText;
              }
              break;
            }
            
            case model::GoAstNode::SymbolType::Reference:
            {
              return_["Reference To"] = node.defId;
              
              // Could potentially resolve the definition and add its information
              if (!node.defId.empty()) {
                // Try to find referenced node
                try {
                  AstResult refResult = _db->query<model::GoAstNode>(
                    AstQuery::jsonId == node.defId);
                  
                  if (!refResult.empty()) {
                    const model::GoAstNode& refNode = *refResult.begin();
                    return_["References"] = refNode.name;
                    return_["Referenced Type"] = model::symbolTypeToString(refNode.symbolType);
                  }
                } catch (const std::exception& ex) {
                  // Reference couldn't be resolved
                }
              }
              break;
            }
            
            case model::GoAstNode::SymbolType::Other:
            {
              // Check if it's a constant
              ConstResult constants = _db->query<model::GoConstant>(
                ConstQuery::entityHash == node.entityHash);
                
              if (!constants.empty())
              {
                model::GoConstant constant = *constants.begin();
                
                return_["Type"] = "Constant";
                
                if (!constant.type.empty())
                  return_["Value Type"] = constant.type;
                  
                if (!constant.value.empty())
                  return_["Value"] = constant.value;
                  
                return_["Index"] = std::to_string(constant.index);
                
                if (!constant.docText.empty())
                  return_["Documentation"] = constant.docText;
              }
              
              // Check if it's a statement
              StmtResult statements = _db->query<model::GoStatement>(
                StmtQuery::entityHash == node.entityHash);
                
              if (!statements.empty())
              {
                model::GoStatement stmt = *statements.begin();
                
                return_["Type"] = "Statement";
                return_["Statement Kind"] = goStatementKindToString(stmt.kind);
                
                // Statement-specific properties based on kind
                switch (stmt.kind)
                {
                  case model::GoStatementKind::If:
                    if (!stmt.condition.empty())
                      return_["Condition"] = stmt.condition;
                    if (!stmt.initialization.empty())
                      return_["Initialization"] = stmt.initialization;
                    break;
                    
                  case model::GoStatementKind::For:
                    if (!stmt.initialization.empty())
                      return_["Initialization"] = stmt.initialization;
                    if (!stmt.condition.empty())
                      return_["Condition"] = stmt.condition;
                    if (!stmt.post.empty())
                      return_["Post"] = stmt.post;
                    break;
                    
                  case model::GoStatementKind::Range:
                    if (!stmt.collection.empty())
                      return_["Collection"] = stmt.collection;
                    if (!stmt.key.empty())
                      return_["Key"] = stmt.key;
                    if (!stmt.value.empty())
                      return_["Value"] = stmt.value;
                    break;
                    
                  case model::GoStatementKind::Case:
                    return_["Is Default"] = stmt.isDefault ? "true" : "false";
                    if (!stmt.caseValues.empty())
                      return_["Case Values"] = stmt.caseValues;
                    if (!stmt.caseTypes.empty())
                      return_["Case Types"] = stmt.caseTypes;
                    break;
                    
                  case model::GoStatementKind::Defer:
                  case model::GoStatementKind::Go:
                    if (!stmt.call.empty())
                      return_["Call"] = stmt.call;
                    break;
                    
                  case model::GoStatementKind::Panic:
                    if (!stmt.argument.empty())
                      return_["Argument"] = stmt.argument;
                    break;
                    
                  default:
                    break;
                }
                
                // Child count
                if (!stmt.children.empty())
                  return_["Child Count"] = std::to_string(stmt.children.size());
                  
                return_["Statement Index"] = std::to_string(stmt.index);
              }
              break;
            }
          }
          
          // Add location information
          if (node.location.file) {
            return_["File"] = node.location.file.load()->path;
            return_["Line"] = std::to_string(node.location.range.start.line);
            return_["Column"] = std::to_string(node.location.range.start.column);
            return_["End Line"] = std::to_string(node.location.range.end.line);
            return_["End Column"] = std::to_string(node.location.range.end.column);
          }
        });
      }

      void GoServiceHandler::getDiagramTypes(
          std::map<std::string, int32_t> &_return,
          const core::AstNodeId &astNodeId_)
      {
        std::vector<AstNodeInfo> definitions;
        getReferences(definitions, astNodeId_, DEFINITION, {});

        core::AstNodeId astNodeId = astNodeId_;
        if (!definitions.empty())
          astNodeId = definitions.front().id;

        model::GoAstNode node = queryGoAstNode(astNodeId);

        switch (node.symbolType)
        {
        case model::GoAstNode::SymbolType::Function:
          _return["Function call diagram"] = FUNCTION_CALL;
          break;
        }
      }

      void GoServiceHandler::getDiagram(
          std::string &_return,
          const core::AstNodeId &astNodeId_,
          const int32_t diagramId_)
      {
        util::Graph graph = returnDiagram(astNodeId_, diagramId_);

        if (graph.nodeCount() != 0)
          _return = graph.output(util::Graph::SVG);
      }

      util::Graph GoServiceHandler::returnDiagram(
          const core::AstNodeId &astNodeId_,
          const std::int32_t diagramId_)
      {
        GoDiagram diagram(_db, _datadir, _context);
        util::Graph graph;

        switch (diagramId_)
        {
        case FUNCTION_CALL:
          diagram.getFunctionCallDiagram(graph, astNodeId_);
          break;
        }

        return graph;
      }

      void GoServiceHandler::getDiagramLegend(
        std::string& return_,
        const std::int32_t diagramId_)
      {
        GoDiagram diagram(_db, _datadir, _context);
      
        switch (diagramId_)
        {
          case FUNCTION_CALL:
            return_ = diagram.getFunctionCallLegend();
            break;
        }
      }

      void GoServiceHandler::getFileDiagramTypes(std::map<std::string, int32_t> &_return, const core::FileId &fileId)
      {
      }

      void GoServiceHandler::getFileDiagram(std::string &_return, const core::FileId &fileId, const int32_t diagramId)
      {
      }

      void GoServiceHandler::getFileDiagramLegend(std::string &_return, const int32_t diagramId)
      {
      }

      void GoServiceHandler::getReferenceTypes(
          std::map<std::string, int32_t> &_return,
          const core::AstNodeId &astNodeId_)
      {
        model::GoAstNode node = queryGoAstNode(astNodeId_);

        _return["Definition"] = DEFINITION;
        _return["Usage"] = USAGE;

        switch (node.symbolType)
        {
        case model::GoAstNode::SymbolType::Function:
          _return["This calls"] = THIS_CALLS;
          _return["Callee"] = CALLEE;
          _return["Caller"] = CALLER;
          break;
        }
      }

      int32_t GoServiceHandler::getReferenceCount(
          const core::AstNodeId &astNodeId_,
          const int32_t referenceId_)
      {
        model::GoAstNode node = queryGoAstNode(astNodeId_);

        return _transaction([&, this]() -> std::int32_t
                            {
    switch (referenceId_)
    {
      case DEFINITION:
        return queryGoAstNodeCount(astNodeId_,
          AstQuery::symbolType == model::GoAstNode::SymbolType::Function ||
          AstQuery::symbolType == model::GoAstNode::SymbolType::Package ||
          AstQuery::symbolType == model::GoAstNode::SymbolType::Type ||
          AstQuery::symbolType == model::GoAstNode::SymbolType::Variable);

      case USAGE:
        return queryGoAstNodeCount(astNodeId_);

        case THIS_CALLS:
        return queryCallsCount(astNodeId_);

      case CALLS_OF_THIS:
        return queryGoAstNodeCount(astNodeId_,
          AstQuery::symbolType == model::GoAstNode::SymbolType::Reference);

      case CALLEE:
      {
        std::int32_t count = 0;

        std::set<std::uint64_t> defHashes;
        for (const model::GoAstNode& call : queryCalls(astNodeId_))
        {
          model::GoAstNode node = queryGoAstNode(std::to_string(call.id));
          defHashes.insert(node.entityHash);
        }

        if (!defHashes.empty())
          count += _db->query_value<model::GoAstCount>(
            AstQuery::entityHash.in_range(
              defHashes.begin(), defHashes.end()) &&
            AstQuery::location.range.end.line != model::Position::npos).count;

        return count;
      }

      case CALLER:
      {
        std::vector<AstNodeInfo> references;
        getReferences(references, astNodeId_, CALLER, {});
        return references.size();
      }
    } });
      }

      void GoServiceHandler::getReferences(
          std::vector<AstNodeInfo> &_return,
          const core::AstNodeId &astNodeId_,
          const int32_t referenceId_,
          const std::vector<std::string> &tags_)
      {
        std::vector<model::GoAstNode> nodes;

        _transaction([&, this]()
                     {
    switch (referenceId_)
    {
      case DEFINITION:
        nodes = queryDefinitions(astNodeId_);
        break;

      case USAGE:
        nodes = queryGoAstNodes(astNodeId_);
        break;

      case THIS_CALLS:
        nodes = queryCalls(astNodeId_);
        break;

      case CALLS_OF_THIS:
        nodes = queryGoAstNodes(
          astNodeId_,
          AstQuery::symbolType == model::GoAstNode::SymbolType::Reference);
        break;

      case CALLEE:
        for (const model::GoAstNode& call : queryCalls(astNodeId_))
        {
          core::AstNodeId astNodeId = std::to_string(call.id);
          std::vector<model::GoAstNode> defs = queryDefinitions(astNodeId);
          nodes.insert(nodes.end(), defs.begin(), defs.end());
        }

        std::sort(nodes.begin(), nodes.end());
        nodes.erase(std::unique(nodes.begin(), nodes.end()), nodes.end());

        break;

      case CALLER:
        for (const model::GoAstNode& astNode : queryGoAstNodes(
          astNodeId_,
          AstQuery::symbolType == model::GoAstNode::SymbolType::Reference))
        {
          const model::Position& start = astNode.location.range.start;
          const model::Position& end   = astNode.location.range.end;

          AstResult result = _db->query<model::GoAstNode>(
            AstQuery::symbolType == model::GoAstNode::SymbolType::Function &&
            // Same file
            AstQuery::location.file == astNode.location.file.object_id() &&
            // StartPos >= Pos
            ((AstQuery::location.range.start.line == start.line &&
              AstQuery::location.range.start.column <= start.column) ||
             AstQuery::location.range.start.line < start.line) &&
            // Pos > EndPos
            ((AstQuery::location.range.end.line == end.line &&
              AstQuery::location.range.end.column > end.column) ||
             AstQuery::location.range.end.line > end.line));

          nodes.insert(nodes.end(), result.begin(), result.end());
        }

        std::sort(nodes.begin(), nodes.end());
        nodes.erase(std::unique(nodes.begin(), nodes.end()), nodes.end());

        break;
    }

    _return.reserve(nodes.size());
    _transaction([this, &_return, &nodes](){
      std::transform(
        nodes.begin(), nodes.end(),
        std::back_inserter(_return),
        CreateAstNodeInfo(getTags(nodes)));
    }); });
      }

      void GoServiceHandler::getReferencesInFile(
          std::vector<AstNodeInfo> &_return,
          const core::AstNodeId &astNodeId_,
          const std::int32_t referenceId_,
          const core::FileId &fileId_,
          const std::vector<std::string> &tags_)
      {
        _transaction([&, this]()
                     {
    model::GoAstNode node = queryGoAstNode(astNodeId_);
    
    // Query nodes based on reference type
    std::vector<model::GoAstNode> nodes;
    
    switch (referenceId_)
    {
      case DEFINITION:
        nodes = queryGoAstNodes(
          astNodeId_,
          AstQuery::location.file == std::stoull(fileId_) &&
          (AstQuery::symbolType == model::GoAstNode::SymbolType::Function ||
           AstQuery::symbolType == model::GoAstNode::SymbolType::Package ||
           AstQuery::symbolType == model::GoAstNode::SymbolType::Type ||
           AstQuery::symbolType == model::GoAstNode::SymbolType::Variable));
        break;
        
      case USAGE:
        nodes = queryGoAstNodes(
          astNodeId_,
          AstQuery::location.file == std::stoull(fileId_));
        break;
        
      case THIS_CALLS:
        {
          std::vector<model::GoAstNode> funcNodes = queryDefinitions(astNodeId_);
          if (!funcNodes.empty())
          {
            model::GoAstNode funcNode = funcNodes.front();
            AstResult result = _db->query<model::GoAstNode>(
              AstQuery::location.file == std::stoull(fileId_) &&
              astCallsQuery(funcNode));
            nodes.assign(result.begin(), result.end());
          }
        }
        break;
    }
    
    _return.reserve(nodes.size());
    std::transform(
      nodes.begin(), nodes.end(),
      std::back_inserter(_return),
      CreateAstNodeInfo(getTags(nodes))); });
      }

      void GoServiceHandler::getReferencesPage(
          std::vector<AstNodeInfo> & /* return_ */,
          const core::AstNodeId & /* astNodeId_ */,
          const std::int32_t /* referenceId_ */,
          const std::int32_t /* pageSize_ */,
          const std::int32_t /* pageNo_ */)
      {
        // TODO
      }

      void GoServiceHandler::getFileReferenceTypes(
          std::map<std::string, int32_t> &_return,
          const core::FileId &fileId_)
      {
        model::FilePtr file = _transaction([&, this]()
                                           { return _db->query_one<model::File>(
                                                 FileQuery::id == std::stoull(fileId_)); });

        if (file && file->type == "GO")
        {
          _return["Types"] = 1;
          _return["Functions"] = 2;
          _return["Variables"] = 3;
          _return["Imports"] = 4;
        }
      }
      
      void GoServiceHandler::getFileReferences(
          std::vector<AstNodeInfo> &_return,
          const core::FileId &fileId_,
          const std::int32_t referenceId_)
      {
        std::vector<model::GoAstNode> nodes;

        _transaction([&, this]()
                     {
    switch (referenceId_)
    {
      case 1: // Types
        {
          AstResult result = _db->query<model::GoAstNode>(
            AstQuery::location.file == std::stoull(fileId_) &&
            AstQuery::symbolType == model::GoAstNode::SymbolType::Type);
          nodes.assign(result.begin(), result.end());
        }
        break;
        
      case 2: // Functions
        {
          AstResult result = _db->query<model::GoAstNode>(
            AstQuery::location.file == std::stoull(fileId_) &&
            AstQuery::symbolType == model::GoAstNode::SymbolType::Function);
          nodes.assign(result.begin(), result.end());
        }
        break;
        
      case 3: // Variables
        {
          AstResult result = _db->query<model::GoAstNode>(
            AstQuery::location.file == std::stoull(fileId_) &&
            AstQuery::symbolType == model::GoAstNode::SymbolType::Variable);
          nodes.assign(result.begin(), result.end());
        }
        break;
        
      case 4: // Imports
        {
          AstResult result = _db->query<model::GoAstNode>(
            AstQuery::location.file == std::stoull(fileId_) &&
            AstQuery::symbolType == model::GoAstNode::SymbolType::Import);
          nodes.assign(result.begin(), result.end());
        }
        break;
    }

    std::sort(nodes.begin(), nodes.end(), 
      [](const model::GoAstNode& lhs, const model::GoAstNode& rhs) {
        return lhs.name < rhs.name;
      });

    _return.reserve(nodes.size());
    _transaction([this, &_return, &nodes](){
      std::transform(
        nodes.begin(), nodes.end(),
        std::back_inserter(_return),
        CreateAstNodeInfo(getTags(nodes)));
    }); });
      }

      std::int32_t GoServiceHandler::getFileReferenceCount(
          const core::FileId &fileId_,
          const std::int32_t referenceId_)
      {
        return _transaction([&, this]() -> std::int32_t
                            {
    switch (referenceId_)
    {
      case 1: // Types
        return _db->query_value<model::GoAstCount>(
          AstQuery::location.file == std::stoull(fileId_) &&
          AstQuery::symbolType == model::GoAstNode::SymbolType::Type).count;
        
      case 2: // Functions
        return _db->query_value<model::GoAstCount>(
          AstQuery::location.file == std::stoull(fileId_) &&
          AstQuery::symbolType == model::GoAstNode::SymbolType::Function).count;
        
      case 3: // Variables
        return _db->query_value<model::GoAstCount>(
          AstQuery::location.file == std::stoull(fileId_) &&
          AstQuery::symbolType == model::GoAstNode::SymbolType::Variable).count;
        
      case 4: // Imports
        return _db->query_value<model::GoAstCount>(
          AstQuery::location.file == std::stoull(fileId_) &&
          AstQuery::symbolType == model::GoAstNode::SymbolType::Import).count;
        
      default:
        return 0;
    } });
      }

      void GoServiceHandler::getSyntaxHighlight(
          std::vector<SyntaxHighlight> &_return,
          const core::FileRange &range_)
      {
        std::vector<std::string> content;

        _transaction([&, this]()
                     {
    model::FilePtr file = _db->query_one<model::File>(
      FileQuery::id == std::stoull(range_.file));

    if (!file || !file->content.load())
      return;

    std::istringstream s(file->content->content);
    std::string line;
    while (std::getline(s, line))
      content.push_back(line);

    // Iterate over AST nodes in the range
    for (const model::GoAstNode& node : _db->query<model::GoAstNode>(
      AstQuery::location.file == std::stoull(range_.file) &&
      AstQuery::location.range.start.line >= range_.range.startpos.line &&
      AstQuery::location.range.end.line < range_.range.endpos.line &&
      AstQuery::location.range.end.line != model::Position::npos))
    {
      if (node.name.empty())
        continue;

      // Regular expression to find element position
      const std::regex specialChars { R"([-[\]{}()*+?.,\^$|#\s])" };
      std::string sanitizedNodeName = std::regex_replace(node.name,
                                                        specialChars,
                                                        R"(\$&)");
      std::string reg = "\\b" + sanitizedNodeName + "\\b";

      for (std::size_t i = node.location.range.start.line - 1;
           i < node.location.range.end.line && i < content.size();
           ++i)
      {
        std::regex words_regex(reg);
        auto words_begin = std::sregex_iterator(
          content[i].begin(), content[i].end(),
          words_regex);
        auto words_end = std::sregex_iterator();

        for (std::sregex_iterator ri = words_begin; ri != words_end; ++ri)
        {
          SyntaxHighlight syntax;
          syntax.range.startpos.line = i + 1;
          syntax.range.startpos.column = ri->position() + 1;
          syntax.range.endpos.line = i + 1;
          syntax.range.endpos.column =
            syntax.range.startpos.column + node.name.length();

          std::string symbolClass =
            "cm-" + model::symbolTypeToString(node.symbolType);
          syntax.className = symbolClass;

          _return.push_back(std::move(syntax));
        }
      }
    } });
      }

      std::vector<model::GoAstNode> GoServiceHandler::queryDefinitions(
          const core::AstNodeId &astNodeId_)
      {
        return queryGoAstNodes(
            astNodeId_,
            AstQuery::symbolType == model::GoAstNode::SymbolType::Function ||
                AstQuery::symbolType == model::GoAstNode::SymbolType::Package ||
                AstQuery::symbolType == model::GoAstNode::SymbolType::Type ||
                AstQuery::symbolType == model::GoAstNode::SymbolType::Variable);
      }

      model::GoAstNode GoServiceHandler::queryGoAstNode(
          const core::AstNodeId &astNodeId_)
      {
        return _transaction([&, this]()
                            {
    model::GoAstNode node;

    if (!_db->find(std::stoull(astNodeId_), node))
    {
      core::InvalidId ex;
      ex.__set_msg("Invalid GoAstNode ID");
      ex.__set_nodeid(astNodeId_);
      throw ex;
    }

    return node; });
      }

      std::vector<model::GoAstNode> GoServiceHandler::queryGoAstNodes(
          const core::AstNodeId &astNodeId_,
          const AstQuery &query_)
      {
        model::GoAstNode node = queryGoAstNode(astNodeId_);

        AstResult result = _db->query<model::GoAstNode>(
            AstQuery::entityHash == node.entityHash &&
            AstQuery::location.range.end.line != model::Position::npos &&
            query_);

        return std::vector<model::GoAstNode>(result.begin(), result.end());
      }

      std::size_t GoServiceHandler::queryGoAstNodeCount(
          const core::AstNodeId &astNodeId_,
          const AstQuery &query_)
      {
        model::GoAstNode node = queryGoAstNode(astNodeId_);

        model::GoAstCount q = _db->query_value<model::GoAstCount>(
            AstQuery::entityHash == node.entityHash &&
            AstQuery::location.range.end.line != model::Position::npos &&
            query_);

        return q.count;
      }

      std::vector<model::GoAstNode> GoServiceHandler::queryCalls(
          const core::AstNodeId &astNodeId_)
      {
        std::vector<model::GoAstNode> nodes = queryDefinitions(astNodeId_);

        if (nodes.empty())
          return nodes;

        model::GoAstNode node = nodes.front();
        AstResult result = _db->query<model::GoAstNode>(astCallsQuery(node));

        nodes = std::vector<model::GoAstNode>(result.begin(), result.end());

        return nodes;
      }

      std::size_t GoServiceHandler::queryCallsCount(
          const core::AstNodeId &astNodeId_)
      {
        std::vector<model::GoAstNode> nodes = queryDefinitions(astNodeId_);

        if (nodes.empty())
          return std::size_t(0);

        model::GoAstNode node = nodes.front();

        return _db->query_value<model::GoAstCount>(astCallsQuery(node)).count;
      }

      std::map<model::GoAstNodeId, std::vector<std::string>>
      GoServiceHandler::getTags(const std::vector<model::GoAstNode> &nodes_)
      {
        std::map<model::GoAstNodeId, std::vector<std::string>> tags;

        for (const model::GoAstNode &node : nodes_)
        {
          tags[node.id];
        }

        return tags;
      }

      odb::query<model::GoAstNode> GoServiceHandler::astCallsQuery(
          const model::GoAstNode &astNode_)
      {
        const model::Position &start = astNode_.location.range.start;
        const model::Position &end = astNode_.location.range.end;

        return (AstQuery::location.file == astNode_.location.file.object_id() &&
                AstQuery::symbolType == model::GoAstNode::SymbolType::Reference &&
                AstQuery::defId + "LIKE" + AstQuery::_val("\%Function\%") &&
                // StartPos >= Pos
                ((AstQuery::location.range.start.line == start.line &&
                  AstQuery::location.range.start.column >= start.column) ||
                 AstQuery::location.range.start.line > start.line) &&
                // Pos > EndPos
                ((AstQuery::location.range.end.line == end.line &&
                  AstQuery::location.range.end.column < end.column) ||
                 AstQuery::location.range.end.line < end.line));
      }

    } // language
  } // service
} // cc