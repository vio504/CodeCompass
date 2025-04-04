#include <goparser/goparser.h>

#include <boost/filesystem.hpp>
#include <boost/process.hpp>

#include <util/logutil.h>
#include <util/parserutil.h>

#include <memory>

namespace cc
{
namespace parser
{

GoParser::GoParser(ParserContext& ctx_): AbstractParser(ctx_)
{
}

bool GoParser::accept(const std::string& path_)
{
  std::string ext = boost::filesystem::extension(path_);
  return ext == ".go";
}

bool GoParser::parseToJson(const std::string& path_)
{
  if (!accept(path_))
    return true;

  std::string outputDir = "~/cc/workdir/output/";
  
  if (outputDir.size() > 0 && outputDir[0] == '~') {
    const char* homeDir = std::getenv("HOME");
    if (homeDir) {
      outputDir.replace(0, 1, homeDir);
    }
  } 

  if (!boost::filesystem::exists(outputDir)) {
    boost::filesystem::create_directories(outputDir);
  }
  
  std::string filename = boost::filesystem::path(path_).filename().string();
  std::string output = outputDir + filename + ".json";
  
  LOG(info) << "Saving output to: " << output;

  namespace bp = boost::process;
  
  try {
    bp::ipstream processOutput;
    bp::child c(
      bp::exe("exast"),
      bp::args={path_, output},
      bp::std_out > processOutput
    );
    
    c.wait();
    return c.exit_code() == 0;
  }
  catch (const bp::process_error& e) {
    LOG(error) << "Error executing exast: " << e.what();
    return false;
  }
}

bool GoParser::parse()
{        
  for (const std::string& path :
    _ctx.options["input"].as<std::vector<std::string>>())
  {
    LOG(info) << "GoParser parse path: " << path;
    util::iterateDirectoryRecursive(path, parseToJson);
  }
  return true;
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

    description.add_options()
        ("go-arg", po::value<std::string>()->default_value("Go arg"),
          "This argument will be used by the go parser.")
        ("output-dir", po::value<std::string>()->default_value("~/cc/workdir/output/"),
          "Directory where output files will be saved.");

    return description;
  }

  std::shared_ptr<GoParser> make(ParserContext& ctx_)
  {
    return std::make_shared<GoParser>(ctx_);
  }
}
#pragma clang diagnostic pop

} // parser
} // cc

