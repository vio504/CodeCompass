#include <goparser/goparser.h>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <odb/database.hxx>
#include <parser/sourcemanager.h>

#include <util/logutil.h>
#include <util/parserutil.h>
#include <util/hash.h>
#include <util/odbtransaction.h>

#include <model/file.h>
#include <model/fileloc.h>
#include <model/buildsourcetarget.h>
#include <model/buildsourcetarget-odb.hxx>
#include <model/position.h>

#include <model/goastnode.h>
#include <model/goastnode-odb.hxx>
#include <model/goentity.h>
#include <model/goentity-odb.hxx>
#include <model/gofunction.h>
#include <model/gofunction-odb.hxx>
#include <model/govariable.h>
#include <model/govariable-odb.hxx>
#include <model/gopackage.h>
#include <model/gopackage-odb.hxx>
#include <model/goimport.h>
#include <model/goimport-odb.hxx>
#include <model/goconstant.h>
#include <model/goconstant-odb.hxx>
#include <model/gomethod.h>
#include <model/gomethod-odb.hxx>
#include <model/gostatement.h>
#include <model/gostatement-odb.hxx>
#include <model/gotype.h>
#include <model/gotype-odb.hxx>
#include <model/goenum.h>
#include <model/goenum-odb.hxx>
#include <model/gointerface.h>
#include <model/gointerface-odb.hxx>
#include <model/gostruct.h>
#include <model/gostruct-odb.hxx>

#include <memory>
#include <unordered_map>
#include <fstream>

namespace cc
{
  namespace parser
  {

    GoParser::GoParser(ParserContext &ctx_)
        : AbstractParser(ctx_),
          _goSourceType("GO")
    {
    }

    bool GoParser::accept(const std::string &path_)
    {
      std::string ext = boost::filesystem::extension(path_);
      return ext == ".go";
    }

    bool GoParser::parseToJson(const std::string &path_)
    {
      std::string output = path_ + ".json";

      std::string command = "exast";
      command += ' ' + path_;
      command += ' ' + output;

      int result = std::system(command.c_str());

      if (result != 0)
      {
        LOG(error) << "Failed to execute external parser for: " << path_;
        return false;
      }

      return true;
    }

    bool GoParser::parse()
    {
      std::vector<std::string> jsonFiles;

      // Parse Go files to JSON
      for (const std::string &path :
           _ctx.options["input"].as<std::vector<std::string>>())
      {
        LOG(info) << "GoParser parse path: " << path;
        util::iterateDirectoryRecursive(path, [this, &jsonFiles](const std::string &path)
                                        {
      if (!accept(path))
        return true;

      model::FilePtr file = _ctx.srcMgr.getFile(path);
      file->parseStatus = model::File::PSFullyParsed;
      file->type = this->_goSourceType;
      _ctx.srcMgr.updateFile(*file);

      if (parseToJson(path)) {
        std::string jsonPath = path + ".json";
        jsonFiles.push_back(jsonPath);
        return true;
      }

      return false; });
      }

      // Process JSON files and populate database
      bool success = true;
      for (const auto &jsonFile : jsonFiles)
      {
        success = success && processJsonFile(jsonFile);
      }

      return success;
    }

    bool GoParser::processJsonFile(const std::string &jsonPath)
    {
      LOG(info) << "Processing Go JSON file: " << jsonPath;

      boost::property_tree::ptree root;

      try
      {
        boost::property_tree::read_json(jsonPath, root);
      }
      catch (const std::exception &ex)
      {
        LOG(error) << "Failed to parse JSON file: " << ex.what();
        return false;
      }

      std::vector<model::GoAstNodePtr> astNodes;
      std::vector<model::GoPackagePtr> packages;
      std::vector<model::GoFunctionPtr> functions;
      std::vector<model::GoVariablePtr> variables;
      std::vector<model::GoImportPtr> imports;
      std::vector<model::GoConstantPtr> constants;
      std::vector<model::GoMethodPtr> methods;
      std::vector<model::GoStatementPtr> statements;
      std::vector<model::GoTypePtr> types;
      std::vector<model::GoEnumPtr> enums;
      std::vector<model::GoInterfacePtr> interfaces;
      std::vector<model::GoStructPtr> structs;

      // helper map to track variables by their JSON ID
      std::unordered_map<std::string, model::GoVariablePtr> variableMap;

      // Create AST nodes and basic entities
      for (const auto &item : root)
      {
        try
        {
          auto astNode = createAstNode(item.second);
          if (!astNode)
            continue;

          astNodes.push_back(astNode);

          std::string type = item.second.get<std::string>("Type");

          if (type == "Package")
          {
            auto package = createPackage(item.second, astNode);
            if (package)
              packages.push_back(package);
          }
          else if (type == "Function")
          {
            auto function = createFunction(item.second, astNode);
            if (function)
              functions.push_back(function);
          }
          else if (type == "Variable")
          {
            auto variable = createVariable(item.second, astNode);
            if (variable)
            {
              variables.push_back(variable);
              variableMap[item.second.get<std::string>("ID")] = variable;
            }
          }
          else if (type == "Import")
          {
            auto import = createImport(item.second, astNode);
            if (import)
              imports.push_back(import);
          }
          else if (type == "Constant")
          {
            auto constant = createConstant(item.second, astNode);
            if (constant)
              constants.push_back(constant);
          }
          else if (type == "Method")
          {
            auto method = createMethod(item.second, astNode);
            if (method)
              methods.push_back(method);
          }
          else if (type == "Statement")
          {
            auto statement = createStatement(item.second, astNode);
            if (statement)
              statements.push_back(statement);
          }
          else if (type == "Type")
          {
            auto goType = createType(item.second, astNode);
            if (goType)
              types.push_back(goType);
          }
          else if (type == "Enum")
          {
            auto enum_ = createEnum(item.second, astNode);
            if (enum_)
              enums.push_back(enum_);
          }
          else if (type == "Interface")
          {
            auto interface_ = createInterface(item.second, astNode);
            if (interface_)
              interfaces.push_back(interface_);
          }
          else if (type == "Struct")
          {
            auto struct_ = createStruct(item.second, astNode);
            if (struct_)
              structs.push_back(struct_);
          }
        }
        catch (const std::exception &ex)
        {
          LOG(warning) << "Error processing node: " << ex.what();
        }
      }

      // Connect function parameters and results
      for (const auto &func : functions)
      {
        for (const auto &var : variables)
        {
          if (var->parentFunc == func->jsonId)
          {
            if (var->isParameter)
            {
              func->parameters.push_back(var);
            }
            else if (var->isResult)
            {
              func->results.push_back(var);
            }
            else
            {
              func->locals.push_back(var);
            }
          }
        }
      }
      // Process child elements and establish relationships
      // Process enum constants
      for (const auto &item : root)
      {
        try
        {
          std::string type = item.second.get<std::string>("Type");

          if (type == "Enum")
          {
            std::string enumId = item.second.get<std::string>("ID");

            // Find the corresponding enum
            auto it = std::find_if(enums.begin(), enums.end(),
                                   [&enumId](const model::GoEnumPtr &e)
                                   { return e->jsonId == enumId; });

            if (it != enums.end())
            {
              model::GoEnumPtr enum_ = *it;

              // Process constants
              try
              {
                const auto &constantsNode = item.second.get_child("Constants");
                for (const auto &constantItem : constantsNode)
                {
                  auto constant = createEnumConstant(constantItem.second, enum_->id);
                  if (constant)
                  {
                    // Store for persistence
                    enum_->constants.push_back(constant);
                  }
                }
              }
              catch (const std::exception &ex)
              {
                LOG(debug) << "No constants for enum " << enum_->name;
              }
            }
          }
          else if (type == "Interface")
          {
            std::string interfaceId = item.second.get<std::string>("ID");

            // Find the corresponding interface
            auto it = std::find_if(interfaces.begin(), interfaces.end(),
                                   [&interfaceId](const model::GoInterfacePtr &i)
                                   { return i->jsonId == interfaceId; });

            if (it != interfaces.end())
            {
              model::GoInterfacePtr interface_ = *it;

              // Process methods
              try
              {
                const auto &methodsNode = item.second.get_child("Methods");
                for (const auto &methodItem : methodsNode)
                {
                  auto method = createInterfaceMethod(methodItem.second, interface_->id);
                  if (method)
                  {
                    // Store for persistence
                    interface_->methods.push_back(method);
                  }
                }
              }
              catch (const std::exception &ex)
              {
                LOG(debug) << "No methods for interface " << interface_->name;
              }
            }
          }
          else if (type == "Struct")
          {
            std::string structId = item.second.get<std::string>("ID");

            // Find the corresponding struct
            auto it = std::find_if(structs.begin(), structs.end(),
                                   [&structId](const model::GoStructPtr &s)
                                   { return s->jsonId == structId; });

            if (it != structs.end())
            {
              model::GoStructPtr struct_ = *it;

              // Process fields
              try
              {
                const auto &fieldsNode = item.second.get_child("Fields");
                for (const auto &fieldItem : fieldsNode)
                {
                  auto field = createStructField(fieldItem.second, struct_->id);
                  if (field)
                  {
                    // Store for persistence
                    struct_->fields.push_back(field);
                  }
                }
                struct_->fieldCount = struct_->fields.size();
              }
              catch (const std::exception &ex)
              {
                LOG(debug) << "No fields for struct " << struct_->name;
              }
            }
          }
          else if (type == "Statement")
          {
            std::string stmtId = item.second.get<std::string>("ID");

            // Find the corresponding statement
            auto it = std::find_if(statements.begin(), statements.end(),
                                   [&stmtId](const model::GoStatementPtr &s)
                                   { return s->jsonId == stmtId; });

            if (it != statements.end())
            {
              model::GoStatementPtr stmt = *it;

              // Process child statements
              try
              {
                const auto &childrenNode = item.second.get_child("Children");
                for (const auto &childItem : childrenNode)
                {
                  // Find the corresponding child statement by ID
                  std::string childId = childItem.second.get_value<std::string>();
                  auto childIt = std::find_if(statements.begin(), statements.end(),
                                              [&childId](const model::GoStatementPtr &s)
                                              { return s->jsonId == childId; });

                  if (childIt != statements.end())
                  {
                    // Add child reference to parent
                    stmt->children.push_back(*childIt);
                  }
                }
              }
              catch (const std::exception &ex)
              {
                LOG(debug) << "No children for statement " << stmt->jsonId;
              }
            }
          }
        }
        catch (const std::exception &ex)
        {
          LOG(warning) << "Error processing relationships: " << ex.what();
        }
      }

      // Connect methods to their receivers
      for (const auto &method : methods)
      {
        if (!method->receiverType.empty())
        {
          // Find the struct this method belongs to
          auto it = std::find_if(structs.begin(), structs.end(),
                                 [&method](const model::GoStructPtr &s)
                                 {
                                   return s->name == method->receiverType ||
                                          ("*" + s->name) == method->receiverType;
                                 });

          if (it != structs.end())
          {
            //relationship in both directions maybe in the future

          }
        }
      }

      // Save everything to the database
      util::OdbTransaction transaction(_ctx.db);
      transaction([&]
                  {
    _ctx.srcMgr.persistFiles();

    for (const auto& node : astNodes)
      _ctx.db->persist(*node);

    for (const auto& pkg : packages)
      _ctx.db->persist(*pkg);

    for (const auto& import : imports)
      _ctx.db->persist(*import);

    for (const auto& var : variables)
      _ctx.db->persist(*var);

    for (const auto& func : functions)
      _ctx.db->persist(*func);
      for (const auto& constant : constants)
      _ctx.db->persist(*constant);

    for (const auto& method : methods)
      _ctx.db->persist(*method);

    for (const auto& statement : statements)
      _ctx.db->persist(*statement);

    for (const auto& goType : types)
      _ctx.db->persist(*goType);

    for (const auto& enum_ : enums)
      _ctx.db->persist(*enum_);

    for (const auto& interface_ : interfaces)
      _ctx.db->persist(*interface_);

    for (const auto& struct_ : structs)
      _ctx.db->persist(*struct_); });

      return true;
    }

    model::GoAstNodePtr GoParser::createAstNode(const boost::property_tree::ptree &node)
    {
      model::GoAstNodePtr astNode = std::make_shared<model::GoAstNode>();

      astNode->jsonId = node.get<std::string>("ID");
      astNode->name = node.get<std::string>("Name");
      astNode->nodeType = node.get<std::string>("NodeType", "");
      astNode->content = node.get<std::string>("Content", "");
      astNode->signature = node.get<std::string>("Signature", "");
      astNode->package = node.get<std::string>("Package", "");
      astNode->parent = node.get<std::string>("Parent", "");
      astNode->defId = node.get<std::string>("DefID", "");
      astNode->exported = node.get<bool>("Exported", false);

      // Handle symbol type
      std::string typeStr = node.get<std::string>("Type");
      astNode->symbolType = model::symbolTypeFromString(typeStr);

      // Handle visibility
      std::string visibilityStr = node.get<std::string>("Visibility", "private");
      astNode->visibility = model::visibilityFromString(visibilityStr);

      // Handle file location
      std::string filePath = node.get<std::string>("File", "");
      if (!filePath.empty())
      {
        model::FileLoc location;

        // Get file
        location.file = _ctx.srcMgr.getFile(filePath);

        // Set file type to GO if not already set
        if (location.file->type != model::File::DIRECTORY_TYPE &&
            location.file->type != _goSourceType)
        {
          location.file->type = _goSourceType;
          _ctx.srcMgr.updateFile(*location.file);
        }

        // Get position
        location.range.start.line = node.get<unsigned>("Line", 0);
        location.range.start.column = node.get<unsigned>("Column", 0);
        location.range.end.line = node.get<unsigned>("EndLine", 0);
        location.range.end.column = node.get<unsigned>("EndColumn", 0);

        astNode->location = location;
      }

      // Calculate entity hash based on position, name, etc.
      astNode->entityHash = createEntityHash(*astNode);

      // Generate unique ID
      astNode->id = model::createIdentifier(*astNode);

      return astNode;
    }

    model::GoPackagePtr GoParser::createPackage(
        const boost::property_tree::ptree &node,
        model::GoAstNodePtr astNode)
    {
      model::GoPackagePtr package = std::make_shared<model::GoPackage>();

      package->jsonId = astNode->jsonId;
      package->astNodeId = astNode->id;
      package->entityHash = astNode->entityHash;
      package->name = node.get<std::string>("Name");
      package->package = node.get<std::string>("Package", "");
      package->docText = node.get<std::string>("DocText", "");
      package->exported = node.get<bool>("Exported", false);
      package->visibility = node.get<std::string>("Visibility", "");
      package->scope = node.get<std::string>("Scope", "");

      return package;
    }

    model::GoFunctionPtr GoParser::createFunction(
        const boost::property_tree::ptree &node,
        model::GoAstNodePtr astNode)
    {
      model::GoFunctionPtr function = std::make_shared<model::GoFunction>();

      function->jsonId = astNode->jsonId;
      function->astNodeId = astNode->id;
      function->entityHash = astNode->entityHash;
      function->name = node.get<std::string>("Name");
      function->signature = node.get<std::string>("Signature", "");
      function->package = node.get<std::string>("Package", "");
      function->structName = node.get<std::string>("StructName", "");
      function->docText = node.get<std::string>("DocText", "");
      function->exported = node.get<bool>("Exported", false);
      function->visibility = node.get<std::string>("Visibility", "");
      function->scope = node.get<std::string>("Scope", "");

      return function;
    }

    model::GoVariablePtr GoParser::createVariable(
        const boost::property_tree::ptree &node,
        model::GoAstNodePtr astNode)
    {
      model::GoVariablePtr variable = std::make_shared<model::GoVariable>();

      variable->jsonId = astNode->jsonId;
      variable->astNodeId = astNode->id;
      variable->entityHash = astNode->entityHash;
      variable->name = node.get<std::string>("Name");
      variable->package = node.get<std::string>("Package", "");
      variable->structName = node.get<std::string>("StructName", "");
      variable->docText = node.get<std::string>("DocText", "");
      variable->exported = node.get<bool>("Exported", false);
      variable->visibility = node.get<std::string>("Visibility", "");
      variable->scope = node.get<std::string>("Scope", "");
      variable->parentFunc = node.get<std::string>("Parent", "");

      // Parse extra info if available
      try
      {
        const auto &extraInfo = node.get_child("ExtraInfo");

        if (!extraInfo.empty())
        {
          if (extraInfo.get_optional<std::string>("ParameterType"))
          {
            variable->type = extraInfo.get<std::string>("ParameterType");
            variable->isParameter = true;
            if (extraInfo.get_optional<std::string>("Index"))
              variable->paramIndex = std::stoi(extraInfo.get<std::string>("Index"));
          }
          else if (extraInfo.get_optional<std::string>("ResultType"))
          {
            variable->type = extraInfo.get<std::string>("ResultType");
            variable->isResult = true;
            if (extraInfo.get_optional<std::string>("Index"))
              variable->resultIndex = std::stoi(extraInfo.get<std::string>("Index"));
          }
        }
      }
      catch (const std::exception &ex)
      {
        LOG(debug) << "No extra info for variable " << variable->name;
      }

      return variable;
    }

    model::GoImportPtr GoParser::createImport(
        const boost::property_tree::ptree &node,
        model::GoAstNodePtr astNode)
    {
      model::GoImportPtr import = std::make_shared<model::GoImport>();

      import->jsonId = astNode->jsonId;
      import->astNodeId = astNode->id;
      import->entityHash = astNode->entityHash;
      import->name = node.get<std::string>("Name");
      import->package = node.get<std::string>("Package", "");
      import->path = node.get<std::string>("Content", "").substr(1, node.get<std::string>("Content", "").length() - 2); // Remove quotes
      import->docText = node.get<std::string>("DocText", "");

      return import;
    }

    model::GoConstantPtr GoParser::createConstant(
        const boost::property_tree::ptree &node,
        model::GoAstNodePtr astNode)
    {
      model::GoConstantPtr constant = std::make_shared<model::GoConstant>();

      constant->astNodeId = astNode->id;
      constant->entityHash = astNode->entityHash;
      constant->name = node.get<std::string>("Name");
      constant->package = node.get<std::string>("Package", "");
      constant->value = node.get<std::string>("Value", "");
      constant->type = node.get<std::string>("Type", "");
      constant->index = node.get<int>("Index", 0);
      constant->exported = node.get<bool>("Exported", false);

      return constant;
    }
    model::GoMethodPtr GoParser::createMethod(
        const boost::property_tree::ptree &node,
        model::GoAstNodePtr astNode)
    {
      model::GoMethodPtr method = std::make_shared<model::GoMethod>();

      method->jsonId = astNode->jsonId;
      method->astNodeId = astNode->id;
      method->entityHash = astNode->entityHash;
      method->name = node.get<std::string>("Name");
      method->signature = node.get<std::string>("Signature", "");
      method->package = node.get<std::string>("Package", "");
      method->docText = node.get<std::string>("DocText", "");
      method->exported = node.get<bool>("Exported", false);
      method->visibility = node.get<std::string>("Visibility", "");
      method->scope = node.get<std::string>("Scope", "");

      // Method-specific fields
      method->receiverType = node.get<std::string>("ReceiverType", "");
      method->receiverName = node.get<std::string>("ReceiverName", "");
      method->isPointerReceiver = node.get<bool>("IsPointerReceiver", false);

      return method;
    }

    model::GoStatementPtr GoParser::createStatement(
        const boost::property_tree::ptree &node,
        model::GoAstNodePtr astNode)
    {
      model::GoStatementPtr statement = std::make_shared<model::GoStatement>();

      statement->jsonId = astNode->jsonId;
      statement->astNodeId = astNode->id;
      statement->entityHash = astNode->entityHash;
      statement->name = node.get<std::string>("Name", "");
      statement->nodeType = node.get<std::string>("NodeType", "");

      // Parse statement kind
      std::string kindStr = node.get<std::string>("Kind", "Other");
      statement->kind = model::goStatementKindFromString(kindStr);

      // Parse statement-specific fields
      statement->condition = node.get<std::string>("Condition", "");
      statement->initialization = node.get<std::string>("Initialization", "");
      statement->post = node.get<std::string>("Post", "");
      statement->collection = node.get<std::string>("Collection", "");
      statement->key = node.get<std::string>("Key", "");
      statement->value = node.get<std::string>("Value", "");
      statement->call = node.get<std::string>("Call", "");
      statement->argument = node.get<std::string>("Argument", "");

      // Case-specific fields
      statement->isDefault = node.get<bool>("IsDefault", false);
      statement->caseValues = node.get<std::string>("CaseValues", "");
      statement->caseTypes = node.get<std::string>("CaseTypes", "");

      // Position within parent
      statement->index = node.get<int>("Index", 0);

      return statement;
    }

    model::GoTypePtr GoParser::createType(
        const boost::property_tree::ptree &node,
        model::GoAstNodePtr astNode)
    {
      model::GoTypePtr goType = std::make_shared<model::GoType>();

      goType->jsonId = astNode->jsonId;
      goType->astNodeId = astNode->id;
      goType->entityHash = astNode->entityHash;
      goType->name = node.get<std::string>("Name");
      goType->package = node.get<std::string>("Package", "");
      goType->exported = node.get<bool>("Exported", false);

      // Parse type kind
      std::string kindStr = node.get<std::string>("Kind", "Other");
      goType->kind = model::goTypeKindFromString(kindStr);

      // Type-specific fields
      goType->underlyingType = node.get<std::string>("UnderlyingType", "");
      goType->keyType = node.get<std::string>("KeyType", "");
      goType->elementType = node.get<std::string>("ElementType", "");
      goType->valueType = node.get<std::string>("ValueType", "");
      goType->length = node.get<std::string>("Length", "");
      goType->direction = node.get<std::string>("Direction", "");

      return goType;
    }

    model::GoEnumPtr GoParser::createEnum(
        const boost::property_tree::ptree &node,
        model::GoAstNodePtr astNode)
    {
      model::GoEnumPtr enum_ = std::make_shared<model::GoEnum>();

      enum_->jsonId = astNode->jsonId;
      enum_->astNodeId = astNode->id;
      enum_->entityHash = astNode->entityHash;
      enum_->name = node.get<std::string>("Name");
      enum_->package = node.get<std::string>("Package", "");
      enum_->exported = node.get<bool>("Exported", false);
      enum_->type = node.get<std::string>("Type", "int");

      // Process enum constants


      return enum_;
    }

    model::GoInterfacePtr GoParser::createInterface(
        const boost::property_tree::ptree &node,
        model::GoAstNodePtr astNode)
    {
      model::GoInterfacePtr interface_ = std::make_shared<model::GoInterface>();

      interface_->jsonId = astNode->jsonId;
      interface_->astNodeId = astNode->id;
      interface_->entityHash = astNode->entityHash;
      interface_->name = node.get<std::string>("Name");
      interface_->package = node.get<std::string>("Package", "");
      interface_->exported = node.get<bool>("Exported", false);

      // Handle embedded interfaces
      try
      {
        const auto &embeddedNode = node.get_child("EmbeddedInterfaces");
        for (const auto &embedded : embeddedNode)
        {
          interface_->embeddedInterfaces.push_back(embedded.second.get_value<std::string>());
        }
      }
      catch (const std::exception &ex)
      {
        LOG(debug) << "No embedded interfaces for " << interface_->name;
      }

      // Interface methods will be processed in a second pass

      return interface_;
    }

    model::GoStructPtr GoParser::createStruct(
        const boost::property_tree::ptree &node,
        model::GoAstNodePtr astNode)
    {
      model::GoStructPtr struct_ = std::make_shared<model::GoStruct>();

      struct_->jsonId = astNode->jsonId;
      struct_->astNodeId = astNode->id;
      struct_->entityHash = astNode->entityHash;
      struct_->name = node.get<std::string>("Name");
      struct_->package = node.get<std::string>("Package", "");
      struct_->exported = node.get<bool>("Exported", false);

      // Set field count if available
      struct_->fieldCount = node.get<int>("FieldCount", 0);

      // Handle embedded types
      try
      {
        const auto &embeddedNode = node.get_child("EmbeddedTypes");
        for (const auto &embedded : embeddedNode)
        {
          struct_->embeddedTypes.push_back(embedded.second.get_value<std::string>());
        }
      }
      catch (const std::exception &ex)
      {
        LOG(debug) << "No embedded types for " << struct_->name;
      }

      // Struct fields will be processed in a second pass

      return struct_;
    }

    // Helper methods for processing child elements

    model::GoEnumConstantPtr GoParser::createEnumConstant(
        const boost::property_tree::ptree &node,
        std::uint64_t enumId)
    {
      model::GoEnumConstantPtr constant = std::make_shared<model::GoEnumConstant>();

      constant->enumId = enumId;
      constant->name = node.get<std::string>("Name");
      constant->value = node.get<std::string>("Value", "");
      constant->exported = node.get<bool>("Exported", false);
      constant->index = node.get<int>("Index", 0);
      constant->jsonId = node.get<std::string>("ID", "");
      constant->docText = node.get<std::string>("DocText", "");

      // Position information
      constant->file = node.get<std::string>("File", "");
      constant->line = node.get<int>("Line", 0);
      constant->column = node.get<int>("Column", 0);
      constant->endLine = node.get<int>("EndLine", 0);
      constant->endColumn = node.get<int>("EndColumn", 0);

      return constant;
    }

    model::GoInterfaceMethodPtr GoParser::createInterfaceMethod(
        const boost::property_tree::ptree &node,
        std::uint64_t interfaceId)
    {
      model::GoInterfaceMethodPtr method = std::make_shared<model::GoInterfaceMethod>();

      method->interfaceId = interfaceId;
      method->name = node.get<std::string>("Name");
      method->signature = node.get<std::string>("Signature", "");
      method->index = node.get<int>("Index", 0);
      method->subIndex = node.get<int>("SubIndex", 0);
      method->jsonId = node.get<std::string>("ID", "");
      method->docText = node.get<std::string>("DocText", "");

      // Position information
      method->file = node.get<std::string>("File", "");
      method->line = node.get<int>("Line", 0);
      method->column = node.get<int>("Column", 0);
      method->endLine = node.get<int>("EndLine", 0);
      method->endColumn = node.get<int>("EndColumn", 0);

      return method;
    }

    model::GoStructFieldPtr GoParser::createStructField(
        const boost::property_tree::ptree &node,
        std::uint64_t structId)
    {
      model::GoStructFieldPtr field = std::make_shared<model::GoStructField>();

      field->structId = structId;
      field->name = node.get<std::string>("Name");
      field->type = node.get<std::string>("Type", "");
      field->exported = node.get<bool>("Exported", false);
      field->embedded = node.get<bool>("Embedded", false);
      field->index = node.get<int>("Index", 0);
      field->subIndex = node.get<int>("SubIndex", 0);
      field->tags = node.get<std::string>("Tags", "");
      field->jsonId = node.get<std::string>("ID", "");
      field->docText = node.get<std::string>("DocText", "");

      // Position information
      field->file = node.get<std::string>("File", "");
      field->line = node.get<int>("Line", 0);
      field->column = node.get<int>("Column", 0);
      field->endLine = node.get<int>("EndLine", 0);
      field->endColumn = node.get<int>("EndColumn", 0);

      return field;
    }

    std::uint64_t GoParser::createEntityHash(const model::GoAstNode& astNode_)
    {
      std::string res;

      res
        .append(astNode_.name).append(":")
        .append(astNode_.package).append(":");

      return util::fnvHash(res);
    }

    GoParser::~GoParser()
    {
    }

/* These two methods are used by the plugin manager to allow dynamic loading
   of CodeCompass Parser plugins. Clang (>= version 6.0) gives a warning that
   these C-linkage specified methods return types that are not proper from a
   C code.

   These codes are NOT to be called from any C code. The C linkage is used to
   turn off the name mangling so that the dynamic loader can easily find the
   symbol table needed to set the plugin up.
*/
// When writing a plugin, please do NOT copy this notice to your code.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
    extern "C"
    {
      boost::program_options::options_description getOptions()
      {
        boost::program_options::options_description description("Go Plugin");

        description.add_options()("go-path", po::value<std::string>()->default_value(""),
                                  "Path to the Go executable");

        return description;
      }

      std::shared_ptr<GoParser> make(ParserContext &ctx_)
      {
        return std::make_shared<GoParser>(ctx_);
      }
    }
#pragma clang diagnostic pop

  } // parser
} // cc