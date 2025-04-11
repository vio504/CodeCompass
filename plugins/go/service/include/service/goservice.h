#ifndef CC_SERVICE_LANGUAGE_GOSERVICE_H
#define CC_SERVICE_LANGUAGE_GOSERVICE_H

#include <memory>
#include <vector>

#include <boost/program_options/variables_map.hpp>

#include <odb/database.hxx>
#include <util/odbtransaction.h>
#include <webserver/servercontext.h>

#include <GoService.h>


namespace cc
{
namespace service
{
namespace go
{

class GoServiceHandler : virtual public GoServiceIf
{
public:
  GoServiceHandler(
    std::shared_ptr<odb::database> db_,
    std::shared_ptr<std::string> datadir_,
    const cc::webserver::ServerContext& context_);

  void getHelloWorld(std::string& str_);

private:
  std::shared_ptr<odb::database> _db;
  util::OdbTransaction _transaction;

  const boost::program_options::variables_map& _config;
};

} // go
} // service
} // cc

#endif // CC_SERVICE_GO_GOSERVICE_H
