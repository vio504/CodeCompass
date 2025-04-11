import sys
sys.path.append('gen-py')
sys.path.append('thrift-0.9.3/lib/py/build/lib.linux-x86_64-2.7')

from thrift import Thrift
from thrift.transport import THttpClient
from thrift.protocol import TJSONProtocol

from go import GoService  

# --- Server information --- #
host = 'localhost'  
port = 6969         
workspace = 'proj1'  

# --- Create client objects --- #
def create_client(service, service_name):
    """This function initializes the Thrift client and returns the client objects
    to the API."""
    path = '/' + workspace + '/' + service_name
    transport = THttpClient.THttpClient(host, port, path)
    protocol = TJSONProtocol.TJSONProtocol(transport)
    client = service.Client(protocol)
    transport.open()
    return client


goservice = create_client(GoService, 'GoService')

# --- Do the job --- #
def main():
    try:
        # Call getHelloWorld
        result = goservice.getHelloWorld()
        print(f"Go service returned: {result}")
    except Thrift.TException as tx:
        print(f"ERROR: {tx.message}")

if __name__ == "__main__":
    main()