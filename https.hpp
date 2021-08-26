#include "functions.hpp"

/*
HTTP status codes
for more status codes https://en.wikipedia.org/wiki/List_of_HTTP_status_codes
*/
std::unordered_map<unsigned int, std::string> HttpStatusTable =
{
  {101, "Switching Protocols"},
  {201, "Created"},
  {202, "Accepted"},
  {200, "200 OK" },
  {400, "Bad Request"},
  {401, "Unauthorized"},
  {404, "404 Not Found" },
  {408, "Request Timeout"},
  {413, "413 Request Entity Too Large" },
  {500, "500 Internal Server Error" },
  {501, "501 Not Implemented" },
  {502, "Bad Gateway"},
  {503, "Service Unavailable"},
  {505, "505 HTTP Version Not Supported" }
};


typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket>& SSLStream;

//http methods types
enum class HttpMethods{
  GET,
  POST,
};

//each http request returned by parser
struct HttpRequest
{
  HttpMethods method;
  std::string request;
  std::string resource;
  std::string http_version;
  unsigned int status;
};


class HttpRequestParser
{
  public:
    HttpRequestParser(std::string&);
    std::shared_ptr<HttpRequest> GetHttpRequest();

  private:
    std::string m_HttpRequest;
};


class HttpsService
{
  public:
    HttpsService();

    void HttpsHandleRequest(SSLStream);

  private:
    std::string GetIp(SSLStream);
    void ProcessGetRequest(SSLStream);
    std::string GetCGIProgram(std::string);
    void ProcessPostRequest(SSLStream);
    void ExecuteProgram(std::string, std::string);
    void ProcessHeadRequest(SSLStream);
    void ProcessDeleteRequest(SSLStream);
    void ProcessOptionsRequest(SSLStream);
    std::string GetResponseStatus(SSLStream);
    void SendResponse(SSLStream);
    void Finish();

  private:
    boost::asio::streambuf m_Request;
    std::string m_RequestedResource;
    std::unique_ptr<char[]> m_ResourceBuffer;
    unsigned int m_ResponseStatusCode;
    std::size_t m_ResourceSizeInBytes;
    bool m_IsResponseSent;
    std::string m_ServerOptions;
    std::string m_ScriptData;
    std::string m_ContentType;


    const std::string m_DefaultIndexPage = "<html>\n<head>\n<title>esketit</title>\
            </head>\n<body>esketit - the file is not found\n</body>\n</html>"; //if the page is not found generate a default page
};


class HttpsServer
{
  public:
    HttpsServer();
    void Start(unsigned short);
    void Stop();

private:

  void StartAcceptor(unsigned short);

  std::unique_ptr<std::thread> m_Thread;
	std::atomic<bool> m_Stop;
	boost::asio::io_service m_IOService;
};



//a HTTPS connection acceptor
class HttpsAcceptor
{
  public:
    HttpsAcceptor(boost::asio::io_service&, unsigned short);
    void Start();
    void Stop();

  private:
    void AcceptConnection();
    std::string get_password(std::size_t, boost::asio::ssl::context::password_purpose) const;

  private:
    boost::asio::io_service& m_IOService;
    boost::asio::ip::tcp::acceptor m_Acceptor;
    std::atomic<bool> m_IsStopped;
    boost::asio::ssl::context m_SSLContext;
};



/***

HttpsServer - the server listens

***/

HttpsServer::HttpsServer()
{
  m_Stop = false;
}

//start the https server by starting accepting connections
void HttpsServer::Start(unsigned short port)
{
  m_Thread.reset(new std::thread([this, port]() {
     StartAcceptor(port);
  }));

}

void HttpsServer::StartAcceptor(unsigned short port)
{
  HttpsAcceptor acceptor(m_IOService, port);
  // continue thread m_Thread until m_Stop becomes true
  while(!m_Stop.load()) {
    acceptor.Start();
  }
}

//stop the server.
void HttpsServer::Stop()
{
  m_Stop.store(true);
  m_Thread->join();
}


/***

HttpsAcceptor -- accepts requests

***/

//initialize TCP endpoint with IPv4 and ssl context with sslv23 server
HttpsAcceptor::HttpsAcceptor(boost::asio::io_service& ios, unsigned short port):
        m_IOService(ios),
        m_Acceptor(m_IOService, boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4::any(), port)),
        m_IsStopped(false),
        m_SSLContext(boost::asio::ssl::context::sslv23_server)
{
  //setting up the ssl context.
  m_SSLContext.set_options(
      boost::asio::ssl::context::default_workarounds |
      boost::asio::ssl::context::no_sslv2 |
      boost::asio::ssl::context::single_dh_use);

  // set ssl certification file
  m_SSLContext.use_certificate_chain_file(SSL_CERT_PATH);

  std::cerr << SSL_CERT_PATH << std::endl;
  // set ssl private key file
  m_SSLContext.use_private_key_file(SSL_KEY_PATH, boost::asio::ssl::context::pem);

  std::cerr << SSL_KEY_PATH;
}


void HttpsAcceptor::Start()
{
  // listen for clients
  m_Acceptor.listen();
  // create new ssl stream with ioservice & above created context
  boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_stream(m_IOService, m_SSLContext);

  // accept client request
  m_Acceptor.accept(ssl_stream.lowest_layer());

        //  if ( ec == http::error::end_of_stream || ec == asio::ssl::error::stream_truncated) break;

  // create Https service and handle that request
  HttpsService service;
  service.HttpsHandleRequest(ssl_stream);
}

void HttpsAcceptor::Stop()
{
  m_IsStopped.store(true);
}


/***

HttpsService - process the request

***/

HttpRequestParser::HttpRequestParser(std::string& request):
    m_HttpRequest(request)
{}

//HTTP request parser, parses the request made by client
//stores it into HttpRequest structure and return it
std::shared_ptr<HttpRequest> HttpRequestParser::GetHttpRequest()
{
  if(m_HttpRequest.empty())  return nullptr;

  std::string request_method, resource, http_version;
  std::istringstream request_line_stream(m_HttpRequest);
  //extract request method, GET, POST, .....
  request_line_stream >> request_method;
  //extract requested resource
  request_line_stream >> resource;
  //extract HTTP version
  request_line_stream >> http_version;

  std::shared_ptr<HttpRequest> request(new HttpRequest);

  request->resource = std::move(resource);
  request->status = 0;

  if(request_method.compare("GET") == 0){
    request->method = HttpMethods::GET;
  }else if(request_method.compare("POST") == 0){
    request->method = HttpMethods::POST;
  }else{
    request->status = 400;
  }

  if(http_version.compare("HTTP/1.1") == 0){
    request->http_version = "1.1";
  }else{
    request->status = 505;
  }

  request->request = std::move(m_HttpRequest);

  return request;

}


HttpsService::HttpsService():m_Request(4096), m_IsResponseSent(false)
{}


//Handle each HTTP request made by client
void HttpsService::HttpsHandleRequest(SSLStream ssl_stream)
{
  try{
    //perform TLS handshake
    ssl_stream.handshake(boost::asio::ssl::stream_base::server);

    boost::asio::read_until(ssl_stream, m_Request, '\n');

    //get string from read stream
    std::string request_line;
    std::istream request_stream(&m_Request);
    std::getline(request_stream, request_line, '\0');


    //parse the string and get HTTP request
    HttpRequestParser parser(request_line);
    std::shared_ptr<HttpRequest> http_request = parser.GetHttpRequest();

    std::cout<<"[ Handling Client: "<<GetIp(ssl_stream)<<" ]"<<std::endl;
    std::cout<<"Request: {"<<std::endl;
    std::cout<<http_request->request;
    std::cout<<"}"<<std::endl;

    std::istringstream istrstream(http_request->request);
    while(std::getline(istrstream, m_ScriptData)){}
      std::cout<<"ScriptData: "<<m_ScriptData<<std::endl;

    if(http_request->status == 0){
      m_RequestedResource = http_request->resource;
      //handle each method
      switch(http_request->method){
        case HttpMethods::GET :
          ProcessGetRequest(ssl_stream);
          break;
        case HttpMethods::POST :
          ProcessPostRequest(ssl_stream);
          break;
        default: break;
      }
    }else{
      m_ResponseStatusCode = http_request->status;
      if(!m_IsResponseSent) {
        SendResponse(ssl_stream);
    }
        return;
    }
  }catch(boost::system::system_error& ec) {
    std::cout<<"Error occured, Error code = "<<ec.code()
                  <<" Message: "<<ec.what();
  }
}

//returns ip address of connected endpoint
std::string HttpsService::GetIp(SSLStream ssl_stream)
{
  boost::asio::ip::tcp::endpoint ep = ssl_stream.lowest_layer().remote_endpoint();
  boost::asio::ip::address addr = ep.address();
  return std::move(addr.to_string());
}

//process the client GET request with requested resource
void HttpsService::ProcessGetRequest(SSLStream ssl_stream)
{
  //if resource is empty then status code = 400
  if(m_RequestedResource.empty()){
    m_ResponseStatusCode = 400;
    SendResponse(ssl_stream);
    return;
  }

  //if resource contains PHP script, then send it to POST request
  //for preprocessing php script
  if(boost::contains(m_RequestedResource, ".php") ||
     boost::contains(m_RequestedResource, ".cgi") ||
     boost::contains(m_RequestedResource, ".py") ||
     boost::contains(m_RequestedResource, ".pl")){
         ProcessPostRequest(ssl_stream);
         return;
  }

  //if resource is null or / then set resource to index.html file
  if(m_RequestedResource.compare("/") == 0){
    m_RequestedResource = std::string("/index.php");
    std::ofstream outfile(RESOURCE_DIR_PATH + m_RequestedResource, std::ios::out);
    outfile<<m_DefaultIndexPage;
    outfile.close();
  }

  //get full resource file path
  std::string resource_file_path = RESOURCE_DIR_PATH + m_RequestedResource;

  //check if file exists
  //otherwise set status code to 404
  //and send response to client
  if(!boost::filesystem::exists(resource_file_path)){
    m_ResponseStatusCode = 404;
    SendResponse(ssl_stream);
    return;
  }

  //open a requested file stream
  std::ifstream resource_fstream(resource_file_path, std::ifstream::binary);
  //if already open then status code to 500
  if (!resource_fstream.is_open()){
    m_ResponseStatusCode = 500;
    return;
  }

  //find out file size
  resource_fstream.seekg(0, std::ifstream::end);
  m_ResourceSizeInBytes = static_cast<std::size_t>(resource_fstream.tellg());

  //read file into buffer
  m_ResourceBuffer.reset(new char[m_ResourceSizeInBytes]);
  resource_fstream.seekg(std::ifstream::beg);
  resource_fstream.read(m_ResourceBuffer.get(), m_ResourceSizeInBytes);

  //send response with file
  SendResponse(ssl_stream);

}

void HttpsService::ProcessPostRequest(SSLStream ssl_stream)
{
  if(m_RequestedResource.empty()){
    m_ResponseStatusCode = 400;
    return;
  }

  std::string resource_file_path = RESOURCE_DIR_PATH + m_RequestedResource;

  if(!boost::filesystem::exists(resource_file_path)){
    m_ResponseStatusCode = 404;
    SendResponse(ssl_stream);
    return;
  }

  //if resource contains .php substring
  if(boost::contains(m_RequestedResource, ".php")){
    //php command to execute
    std::string cmd = "php -f " + resource_file_path;

    //attach data to be passed to the script
    if(!m_ScriptData.empty())
      cmd = cmd + " \"" + m_ScriptData + "\"";

    //a temporary file where output of an PHP interpretor will be stores
    std::string php_output_file = RESOURCE_DIR_PATH + std::string("/tempfile");

    std::cout<<"Executing command: "<<cmd<<std::endl;

    //run php and get output
    ExecuteProgram(cmd, php_output_file);

    //copy output file contents from buffer to vector
    //for sending as a response to the client
    std::vector<boost::asio::const_buffer> response_buffers;
    if(m_ResourceSizeInBytes > 0){
      response_buffers.push_back(boost::asio::buffer(m_ResourceBuffer.get(), m_ResourceSizeInBytes));
    }

    //remove phpoutputfile
    std::remove(php_output_file.c_str());

    SendResponse(ssl_stream);

  }else if(boost::contains(m_RequestedResource, ".cgi") ||
          boost::contains(m_RequestedResource, ".py") ||
          boost::contains(m_RequestedResource, ".pl")){

           std::string program;// = GetCGIProgram(resource_file_path);

           //if program name is empty and requested source contain .py,
           //then set program to python3 by default
           if(boost::contains(m_RequestedResource, ".py")){
             program = "/usr/bin/python3";
           }

           if(boost::contains(m_RequestedResource, ".pl")){
             program = "/usr/bin/perl";
           }

           //cgi program command to execute on server
           std::string cmd = program + " " + resource_file_path;

           //attach data to be passed to the script
           if(!m_ScriptData.empty())
             cmd = cmd + " \"" + m_ScriptData + "\"";

           std::cout<<"Executing command: "<<cmd<<std::endl;
           //a temporary file where output of an executed program will be stored
           std::string cgi_output_file = RESOURCE_DIR_PATH + std::string("/cgi_tempfile");

           ExecuteProgram(cmd, cgi_output_file);

           //copy output file contents from buffer to vector
           //for sending as a response to the client
           std::vector<boost::asio::const_buffer> response_buffers;
           if(m_ResourceSizeInBytes > 0){
             response_buffers.push_back(boost::asio::buffer(m_ResourceBuffer.get(), m_ResourceSizeInBytes));
           }

           //remove outputfile
           std::remove(cgi_output_file.c_str());

           SendResponse(ssl_stream);

    }
}

void HttpsService::ExecuteProgram(std::string command, std::string outputfile)
{
  boost::process::system(command, boost::process::std_out > outputfile);

  //set requested resouce to outputfile for response to client
  m_RequestedResource = outputfile;

  //read outputfile contents
  std::ifstream resource_fstream(outputfile, std::ifstream::binary);

  if(!resource_fstream.is_open()){
    m_ResponseStatusCode = 500;
    return;
  }

  m_ResponseStatusCode = 200;

  //find out file size
  resource_fstream.seekg(0, std::ifstream::end);
  m_ResourceSizeInBytes = static_cast<std::size_t>(resource_fstream.tellg());

  m_ResourceBuffer.reset(new char[m_ResourceSizeInBytes]);

  //read output file into resource buffer
  resource_fstream.seekg(std::ifstream::beg);
  resource_fstream.read(m_ResourceBuffer.get(), m_ResourceSizeInBytes);
}

void HttpsService::ProcessOptionsRequest(SSLStream ssl_stream)
{
  m_ServerOptions = "GET, POST, HEAD, DELETE, OPTIONS";
  SendResponse(ssl_stream);
}

std::string HttpsService::GetResponseStatus(SSLStream ssl_stream)
{
  std::string response_status;

  auto end = std::chrono::system_clock::now();
  std::time_t end_time = std::chrono::system_clock::to_time_t(end);
  std::string timestr(std::ctime(&end_time));

  ssl_stream.lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_receive);

  auto status_line = HttpStatusTable[m_ResponseStatusCode];

  response_status = std::string("HTTP/1.1 ") + status_line + "\n";
  if(m_ResourceSizeInBytes > 0){
    response_status += std::string("Content-Length: ") +
                       std::to_string(m_ResourceSizeInBytes) + "\n";
  }
  if(!m_ContentType.empty()){
    response_status += m_ContentType + "\n";
  }else{
    response_status += std::string("Content-Type: text/html") + "\n";
  }
  if(!m_ServerOptions.empty()){
    response_status += std::string("Allow: ") + std::move(m_ServerOptions) + "\n";
  }
  response_status += std::string("Server: TinyHttpWebServer/0.0.1") + "\n";
  response_status += std::string("AcceptRanges: bytes") + "\n";
  response_status += std::string("Connection: Closed") + "\n";
  response_status += std::string("Date: ") + timestr + "\n";

  //return std::move(response_status);
  return response_status;
}

void HttpsService::SendResponse(SSLStream ssl_stream)
{
  std::vector<boost::asio::const_buffer> response_buffers;

  m_IsResponseSent = true;

  std::string response_status = GetResponseStatus(ssl_stream);
  response_buffers.push_back(boost::asio::buffer(std::move(response_status)));

  if(m_ResourceSizeInBytes > 0){
    response_buffers.push_back(boost::asio::buffer(m_ResourceBuffer.get(), m_ResourceSizeInBytes));
  }

  //send response to client with data
  boost::asio::write(ssl_stream, response_buffers);

  m_ResourceSizeInBytes = 0;

}

void HttpsService::Finish()
{
  delete this;
}
