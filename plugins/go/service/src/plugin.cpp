#include <webserver/pluginhelper.h>

#include <service/goservice.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
extern "C"
{
  boost::program_options::options_description getOptions()
  {
    namespace po = boost::program_options;

    po::options_description description("Go Plugin");

    description.add_options()
      ("go-result", po::value<std::string>()->default_value("Go result"),
        "This value will be returned by the go service.");

    return description;
  }

  void registerPlugin(
    const cc::webserver::ServerContext& context_,
    cc::webserver::PluginHandler<cc::webserver::RequestHandler>* pluginHandler_)
  {
    cc::webserver::registerPluginSimple(
      context_,
      pluginHandler_,
      CODECOMPASS_SERVICE_FACTORY_WITH_CFG(Go, go),
      "GoService");
  }
}
#pragma clang diagnostic popclear