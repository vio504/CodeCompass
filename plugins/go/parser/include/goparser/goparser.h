#ifndef CC_PARSER_GOPARSER_H
#define CC_PARSER_GOPARSER_H

#include <parser/abstractparser.h>
#include <parser/parsercontext.h>

#include <model/goastnode.h>
#include <model/goentity.h>
#include <model/gofunction.h>
#include <model/govariable.h>
#include <model/gopackage.h>
#include <model/goimport.h>
#include <model/goconstant.h>
#include <model/gomethod.h>
#include <model/gostatement.h>
#include <model/gotype.h>
#include <model/goenum.h>
#include <model/gointerface.h>
#include <model/gostruct.h>

#include <boost/property_tree/ptree.hpp>

namespace cc
{
    namespace parser
    {

        class GoParser : public AbstractParser
        {
        public:
            GoParser(ParserContext &ctx_);
            virtual ~GoParser();
            virtual bool parse() override;

        private:
            static bool accept(const std::string &path_);

            static bool parseToJson(const std::string &path_);

            bool processJsonFile(const std::string &jsonPath);

            model::GoAstNodePtr createAstNode(const boost::property_tree::ptree &node);

            model::GoPackagePtr createPackage(
                const boost::property_tree::ptree &node,
                model::GoAstNodePtr astNode);

            model::GoFunctionPtr createFunction(
                const boost::property_tree::ptree &node,
                model::GoAstNodePtr astNode);

            model::GoVariablePtr createVariable(
                const boost::property_tree::ptree &node,
                model::GoAstNodePtr astNode);

            model::GoImportPtr createImport(
                const boost::property_tree::ptree &node,
                model::GoAstNodePtr astNode);

            model::GoConstantPtr createConstant(
                const boost::property_tree::ptree &node,
                model::GoAstNodePtr astNode);

            model::GoMethodPtr createMethod(
                const boost::property_tree::ptree &node,
                model::GoAstNodePtr astNode);

            model::GoStatementPtr createStatement(
                const boost::property_tree::ptree &node,
                model::GoAstNodePtr astNode);

            model::GoTypePtr createType(
                const boost::property_tree::ptree &node,
                model::GoAstNodePtr astNode);

            model::GoEnumPtr createEnum(
                const boost::property_tree::ptree &node,
                model::GoAstNodePtr astNode);

            model::GoInterfacePtr createInterface(
                const boost::property_tree::ptree &node,
                model::GoAstNodePtr astNode);

            model::GoStructPtr createStruct(
                const boost::property_tree::ptree &node,
                model::GoAstNodePtr astNode);

            model::GoEnumConstantPtr createEnumConstant(
                const boost::property_tree::ptree &node,
                std::uint64_t enumId);

            model::GoInterfaceMethodPtr createInterfaceMethod(
                const boost::property_tree::ptree &node,
                std::uint64_t interfaceId);

            model::GoStructFieldPtr createStructField(
                const boost::property_tree::ptree &node,
                std::uint64_t structId);

            std::uint64_t createEntityHash(
                const model::GoAstNode& astNode_);

            const std::string _goSourceType;
        };

    } // parser
} // cc

#endif // CC_PARSER_GOPARSER_H