#ifndef CC_SERVICE_LANGUAGE_GOSERVICE_H
#define CC_SERVICE_LANGUAGE_GOSERVICE_H

#include <memory>
#include <vector>
#include <map>
#include <unordered_set>
#include <string>

#include <boost/program_options/variables_map.hpp>

#include <odb/database.hxx>

#include <LanguageService.h>

#include <model/goastnode.h>
#include <model/goastnode-odb.hxx>
#include <model/goentity.h>
#include <model/goentity-odb.hxx>
#include <model/gofunction.h>
#include <model/gofunction-odb.hxx>
#include <model/gomethod.h>
#include <model/gomethod-odb.hxx>
#include <model/gostruct.h>
#include <model/gostruct-odb.hxx>
#include <model/gointerface.h>
#include <model/gointerface-odb.hxx>
#include <model/goenum.h>
#include <model/goenum-odb.hxx>
#include <model/gotype.h>
#include <model/gotype-odb.hxx>
#include <model/govariable.h>
#include <model/govariable-odb.hxx>
#include <model/goconstant.h>
#include <model/goconstant-odb.hxx>
#include <model/gostatement.h>
#include <model/gostatement-odb.hxx>
#include <model/gopackage.h>
#include <model/gopackage-odb.hxx>
#include <model/goimport.h>
#include <model/goimport-odb.hxx>

#include <util/odbtransaction.h>
#include <util/graph.h>
#include <webserver/servercontext.h>

namespace cc
{
namespace service
{
namespace language
{

class GoServiceHandler : virtual public LanguageServiceIf
{
public:
  GoServiceHandler(
    std::shared_ptr<odb::database> db_,
    std::shared_ptr<std::string> datadir_,
    const cc::webserver::ServerContext& context_);

  void getGoString(std::string& str_);

  void getFileTypes(std::vector<std::string> & _return);

  void getAstNodeInfo(AstNodeInfo& _return, const core::AstNodeId& astNodeId);

  void getAstNodeInfoByPosition(AstNodeInfo& _return, const core::FilePosition& fpos);

  void getSourceText(std::string& _return, const core::AstNodeId& astNodeId);

  void getDocumentation(std::string& _return, const core::AstNodeId& astNodeId);

  void getProperties(std::map<std::string, std::string> & _return, const core::AstNodeId& astNodeIds);

  void getDiagramTypes(std::map<std::string, int32_t> & _return, const core::AstNodeId& astNodeId);

  void getDiagram(std::string& _return, const core::AstNodeId& astNodeId, const int32_t diagramId);

  void getDiagramLegend(std::string& _return, const int32_t diagramId);

  void getFileDiagramTypes(std::map<std::string, int32_t> & _return, const core::FileId& fileId);

  void getFileDiagram(std::string& _return, const core::FileId& fileId, const int32_t diagramId);

  void getFileDiagramLegend(std::string& _return, const int32_t diagramId);

  void getReferenceTypes(std::map<std::string, int32_t> & _return, const core::AstNodeId& astNodeId);

  int32_t getReferenceCount(const core::AstNodeId& astNodeId, const int32_t referenceId);

  void getReferences(std::vector<AstNodeInfo> & _return, const core::AstNodeId& astNodeId, const int32_t referenceId, const std::vector<std::string> & tags);

  void getReferencesInFile(std::vector<AstNodeInfo> & _return, const core::AstNodeId& astNodeId, const int32_t referenceId, const core::FileId& fileId, const std::vector<std::string> & tags);

  void getReferencesPage(std::vector<AstNodeInfo> & _return, const core::AstNodeId& astNodeId, const int32_t referenceId, const int32_t pageSize, const int32_t pageNo);

  void getFileReferenceTypes(std::map<std::string, int32_t> & _return, const core::FileId& fileId);

  void getFileReferences(std::vector<AstNodeInfo> & _return, const core::FileId& fileId, const int32_t referenceId);

  int32_t getFileReferenceCount(const core::FileId& fileId, const int32_t referenceId);

  void getSyntaxHighlight(std::vector<SyntaxHighlight> & _return, const core::FileRange& range);

  enum ReferenceType
  {
    DEFINITION, /*!< By this option the definition(s) of the AST node can be
      queried. However according to the "one definition rule" a named entity
      can have only one definition, in a parsing several definitions might be
      available. This is the case when the project is built for several targets
      and in the different builds different definitions are defined for an
      entity (e.g. because of an #ifdef section). */

    USAGE, /*!< By this option the usages of the AST node can be queried, i.e.
      the nodes of which the entity hash is identical to the queried one. */

    THIS_CALLS, /*!< Get function calls in a function. WARNING: If the
      definition of the AST node is not unique then it returns the callees of
      one of them. */

    CALLS_OF_THIS, /*!< Get calls of a function. */

    CALLEE, /*!< Get called functions definitions. WARNING: If the definition of
      the AST node is not unique then it returns the callees of one of them. */

    CALLER, /*!< Get caller functions. */

    VIRTUAL_CALL, /*!< A function may be used virtually on a base type object.
      The exact type of the object is based on dynamic information, which can't
      be determined statically. Weak usage returns these possible calls. */

    FUNC_PTR_CALL, /*!< Functions can be assigned to function pointers which
      can be invoked later. This option returns these invocations. */

    PARAMETER, /*!< This option returns the parameters of a function. */

    LOCAL_VAR, /*!< This option returns the local variables of a function. */

    RETURN_TYPE, /*!< This option returns the return type of a function. */

    OVERRIDE, /*!< This option returns the functions which the given function
      overrides. */

    OVERRIDDEN_BY, /*!< This option returns the overrides of a function. */

    READ, /*!< This option returns the places where a variable is read. */

    WRITE, /*!< This option returns the places where a variable is written. */

    TYPE, /*!< This option returns the type of a variable. */

    ALIAS, /*!< Types may have aliases, e.g. by typedefs. */

    INHERIT_FROM, /*!< Types from which the queried type inherits. */

    INHERIT_BY, /*!< Types by which the queried type is inherited. */

    DATA_MEMBER, /*!< Data members of a class. */

    METHOD, /*!< Members of a class. */

    FRIEND, /*!< The friends of a class. */

    UNDERLYING_TYPE, /*!< Underlying type of a typedef. */

    ENUM_CONSTANTS, /*!< Enum constants. */

    EXPANSION, /*!< Macro expansion. */

    UNDEFINITION, /*!< Macro undefinition. */
  };

  enum DiagramType
  {
    FUNCTION_CALL, /*!< In the function call diagram the nodes are functions and
      the edges are the function calls between them. The diagram also displays
      some dynamic information such as virtual function calls. */
  };

private:
  std::vector<model::GoAstNode> queryDefinitions(
    const core::AstNodeId& astNodeId_);

  /**
   * This function returns the corresponding model::GoAstNode to the given AST
   * node.
   */
  model::GoAstNode queryGoAstNode(const core::AstNodeId& astNodeId_);

  /**
   * This function returns the model::GoAstNode objects which meet the
   * requirements of the given query and have the same entity hash as the given
   * AST node.
   */
  std::vector<model::GoAstNode> queryGoAstNodes(
    const core::AstNodeId& astNodeId_,
    const odb::query<model::GoAstNode>& query_
      = odb::query<model::GoAstNode>(true));

    /**
   * This function returns the model::CppAstNode objects which meet the
   * requirements of the given query in the given file.
   */
  std::vector<model::GoAstNode> queryGoAstNodesInFile(
    const core::FileId& fileId_,
    const odb::query<model::GoAstNode>& query_
      = odb::query<model::GoAstNode>(true));

  /*
   * This function returns the number of corresponding model::GoAstNode objects
   * to the given AST which meet the given query condition.
   */
  std::size_t queryGoAstNodeCount(
    const core::AstNodeId& astNodeId_,
    const odb::query<model::GoAstNode>& query_
      = odb::query<model::GoAstNode>(true));
  

  /**
   * This function returns the function calls in a given function.
   * @param astNodeId_ An AST node ID which belongs to a function.
   */
  std::vector<model::GoAstNode> queryCalls(const core::AstNodeId& astNodeId_);

  /**
   * This function returns the number of function calls in a given function.
   * @param astNodeId_ An AST node ID which belongs to a function.
   */
  std::size_t queryCallsCount(
    const core::AstNodeId& astNodeId_);

  /**
   * This function returns an AST query to get the function calls in the given
   * function.
   */
  odb::query<model::GoAstNode> astCallsQuery(
    const model::GoAstNode& astNode_);

  /**
   * This function returns meta information of the AST nodes
   * (e.g. public, static, virtual etc.)
   */
  std::map<model::GoAstNodeId, std::vector<std::string>> getTags(
    const std::vector<model::GoAstNode>& nodes_);

  util::Graph returnDiagram(
    const core::AstNodeId& astNodeId_,
    const std::int32_t diagramId_);

  std::shared_ptr<odb::database> _db;
  std::shared_ptr<std::string> _datadir;
  util::OdbTransaction _transaction;
  const cc::webserver::ServerContext& _context;
};

} // language
} // service
} // cc

#endif // CC_SERVICE_LANGUAGE_GOSSERVICE_H