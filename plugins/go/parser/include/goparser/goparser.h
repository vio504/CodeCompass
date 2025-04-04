#ifndef CC_PARSER_GOPARSER_H
#define CC_PARSER_GOPARSER_H

#include <parser/abstractparser.h>
#include <parser/parsercontext.h>

namespace cc
{
namespace parser
{
  
class GoParser : public AbstractParser
{
public:
  GoParser(ParserContext& ctx_);
  virtual ~GoParser();
  virtual bool parse() override;

private:
  static bool accept(const std::string& path_);
  static bool parseToJson(const std::string& path_);
};
  
} // parser
} // cc

#endif // CC_PLUGINS_PARSER_GOPARSER_H

