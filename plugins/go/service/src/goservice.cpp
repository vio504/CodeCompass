#include <service/goservice.h>
#include <util/dbutil.h>

namespace cc
{
namespace service
{
namespace go
{

GoServiceHandler::GoServiceHandler(
  std::shared_ptr<odb::database> db_,
  std::shared_ptr<std::string> /*datadir_*/,
  const cc::webserver::ServerContext& context_)
    : _db(db_), _transaction(db_), _config(context_.options)
{
}

void GoServiceHandler::getHelloWorld(std::string& str_)
{
  str_ = _config["go-result"].as<std::string>();
}

} // go 
} // service
} // cc
