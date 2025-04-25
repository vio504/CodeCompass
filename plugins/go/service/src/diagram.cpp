#include <model/govariable.h>
#include <model/govariable-odb.hxx>

#include <util/legendbuilder.h>
#include <util/util.h>
#include "diagram.h"

namespace
{

/**
 * This function checks if the given container contains a specified value.
 * @param v_ A container to inspect.
 * @param val_ Value which will be searched.
 * @return True if the container contains the value.
 */
template <typename Cont>
bool contains(const Cont& c_, const typename Cont::value_type& val_)
{
  return std::find(c_.begin(), c_.end(), val_) != c_.end();
}

/**
 * This function wraps the content to a HTML tag and adds attributes to it.
 */
std::string graphHtmlTag(
  const std::string& tag_,
  const std::string& content_,
  const std::string& attr_ = "")
{
  return std::string("<")
    .append(tag_)
    .append(" ")
    .append(attr_)
    .append(">")
    .append(content_)
    .append("</")
    .append(tag_)
    .append(">");
}

}

namespace cc
{
namespace service
{
namespace language
{

GoDiagram::GoDiagram(
  std::shared_ptr<odb::database> db_,
  std::shared_ptr<std::string> datadir_,
  const cc::webserver::ServerContext& context_)
    : _goHandler(db_, datadir_, context_),
      _projectHandler(db_, datadir_, context_)
{
}

void GoDiagram::getFunctionCallDiagram(
  util::Graph& graph_,
  const core::AstNodeId& astNodeId_)
{
  std::map<core::AstNodeId, util::Graph::Node> visitedNodes;
  std::vector<AstNodeInfo> nodes;

  graph_.setAttribute("rankdir", "LR");

  //--- Center node ---//

  _goHandler.getReferences(
    nodes, astNodeId_, GoServiceHandler::DEFINITION, {});

  if (nodes.empty())
    return;

  util::Graph::Node centerNode = addNode(graph_, nodes.front());
  decorateNode(graph_, centerNode, centerNodeDecoration);
  visitedNodes[astNodeId_] = centerNode;

  //--- Callees ---//

  nodes.clear();
  _goHandler.getReferences(nodes, astNodeId_, GoServiceHandler::CALLEE, {});

  for (const AstNodeInfo& node : nodes)
  {
    util::Graph::Node calleeNode;

    auto it = visitedNodes.find(node.id);
    if (it == visitedNodes.end())
    {
      calleeNode = addNode(graph_, node);
      decorateNode(graph_, calleeNode, calleeNodeDecoration);
      visitedNodes.insert(it, std::make_pair(node.id, calleeNode));
    }
    else
      calleeNode = it->second;

    if (!graph_.hasEdge(centerNode, calleeNode))
    {
      util::Graph::Edge edge = graph_.createEdge(centerNode, calleeNode);
      decorateEdge(graph_, edge, calleeEdgeDecoration);
    }
  }

  //--- Callers ---//

  nodes.clear();
  _goHandler.getReferences(nodes, astNodeId_, GoServiceHandler::CALLER, {});

  for (const AstNodeInfo& node : nodes)
  {
    util::Graph::Node callerNode;

    auto it = visitedNodes.find(node.id);
    if (it == visitedNodes.end())
    {
      callerNode = addNode(graph_, node);
      decorateNode(graph_, callerNode, callerNodeDecoration);
      visitedNodes.insert(it, std::make_pair(node.id, callerNode));
    }
    else
      callerNode = it->second;

    if (!graph_.hasEdge(callerNode, centerNode))
    {
      util::Graph::Edge edge = graph_.createEdge(callerNode, centerNode);
      decorateEdge(graph_, edge, callerEdgeDecoration);
    }
  }

  _subgraphs.clear();
}

std::string GoDiagram::visibilityToHtml(const AstNodeInfo& node_)
{
  if (contains(node_.tags, "public"))
    return graphHtmlTag("font", "+", "color='green'");
  if (contains(node_.tags, "private"))
    return graphHtmlTag("font", "-", "color='red'");
  if (contains(node_.tags, "protected"))
    return graphHtmlTag("font", "#", "color='blue'");

  return "";
}

std::string GoDiagram::memberContentToHtml(
  const AstNodeInfo& node_,
  const std::string& content_)
{
  std::string startTags;
  std::string endTags;

  if (contains(node_.tags, "static"))
  {
    startTags += "<b>";
    endTags.insert(0, "</b>");
  }

  if (contains(node_.tags, "virtual"))
  {
    startTags += "<i>";
    endTags.insert(0, "</i>");
  }

  return startTags + util::escapeHtml(content_) + endTags;
}

std::string GoDiagram::getProperty(
  const core::AstNodeId& astNodeId_,
  const std::string& property_)
{
  std::map<std::string, std::string> properties;
  _goHandler.getProperties(properties, astNodeId_);
  return properties[property_];
}

util::Graph::Node GoDiagram::addNode(
  util::Graph& graph_,
  const AstNodeInfo& nodeInfo_)
{
  util::Graph::Node node
    = graph_.getOrCreateNode(nodeInfo_.id,
      addSubgraph(graph_, nodeInfo_.range.file));

  graph_.setNodeAttribute(node, "label", nodeInfo_.astNodeValue);

  return node;
}

std::string GoDiagram::getFunctionCallLegend()
{
  util::LegendBuilder builder("Function Call Diagram");

  builder.addNode("center function", centerNodeDecoration);
  builder.addNode("called function", calleeNodeDecoration);
  builder.addNode("caller function", callerNodeDecoration);
  builder.addNode("virtual function", virtualNodeDecoration);
  builder.addEdge("called", calleeEdgeDecoration);
  builder.addEdge("caller", callerEdgeDecoration);

  return builder.getOutput();
}

util::Graph::Subgraph GoDiagram::addSubgraph(
  util::Graph& graph_,
  const core::FileId& fileId_)
{
  auto it = _subgraphs.find(fileId_);

  if (it != _subgraphs.end())
    return it->second;

  core::FileInfo fileInfo;
  _projectHandler.getFileInfo(fileInfo, fileId_);

  util::Graph::Subgraph subgraph
    = graph_.getOrCreateSubgraph("cluster_" + fileInfo.path);

  graph_.setSubgraphAttribute(subgraph, "id", fileInfo.id);
  graph_.setSubgraphAttribute(subgraph, "label", fileInfo.path);

  _subgraphs.insert(it, std::make_pair(fileInfo.path, subgraph));

  return subgraph;
}

void GoDiagram::decorateNode(
  util::Graph& graph_,
  const util::Graph::Node& node_,
  const Decoration& decoration_) const
{
  for (const auto& attr : decoration_)
    graph_.setNodeAttribute(node_, attr.first, attr.second);
}

void GoDiagram::decorateEdge(
  util::Graph& graph_,
  const util::Graph::Edge& edge_,
  const Decoration& decoration_) const
{
  for (const auto& attr : decoration_)
    graph_.setEdgeAttribute(edge_, attr.first, attr.second);
}

void GoDiagram::decorateSubgraph(
  util::Graph& graph_,
  const util::Graph::Subgraph& subgraph_,
  const Decoration& decoration_) const
{
  for (const auto& attr : decoration_)
    graph_.setSubgraphAttribute(subgraph_, attr.first, attr.second);
}

const GoDiagram::Decoration GoDiagram::centerNodeDecoration = {
  {"style", "filled"},
  {"fillcolor", "gold"}
};

const GoDiagram::Decoration GoDiagram::calleeNodeDecoration = {
  {"style", "filled"},
  {"fillcolor", "lightblue"}
};

const GoDiagram::Decoration GoDiagram::callerNodeDecoration = {
  {"style", "filled"},
  {"fillcolor", "coral"}
};

const GoDiagram::Decoration GoDiagram::virtualNodeDecoration = {
  {"shape", "diamond"},
  {"style", "filled"},
  {"fillcolor", "cyan"}
};

const GoDiagram::Decoration GoDiagram::calleeEdgeDecoration = {
  {"color", "blue"}
};

const GoDiagram::Decoration GoDiagram::callerEdgeDecoration = {
  {"color", "red"}
};

const GoDiagram::Decoration GoDiagram::centerClassNodeDecoration = {
  {"style", "filled"},
  {"fillcolor", "gold"},
  {"shape", "box"}
};

const GoDiagram::Decoration GoDiagram::classNodeDecoration = {
  {"shape", "box"}
};

const GoDiagram::Decoration GoDiagram::usedClassEdgeDecoration = {
  {"style", "dashed"},
  {"color", "mediumpurple"}
};

const GoDiagram::Decoration GoDiagram::inheritClassEdgeDecoration = {
  {"arrowhead", "empty"}
};

}
}
}
