#include "functions.hpp"


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

  std::cout<<"Server started at address: 127.0.0.1, port: 1234"<<std::endl;
  std::cout<<"Goto https://127.0.0.1:1234/"<<std::endl;
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
  m_SSLContext.use_certificate_chain_file("HttpsWebServer.cert");
  // set ssl private key file
  m_SSLContext.use_private_key_file("HttpsWebServer.key", boost::asio::ssl::context::pem);
}


void HttpsAcceptor::Start()
{
  // listen for clients
  m_Acceptor.listen();
  // create new ssl stream with ioservice & above created context
  boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_stream(m_IOService, m_SSLContext);

  // accept client request
  m_Acceptor.accept(ssl_stream.lowest_layer());

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
  }else if(request_method.compare("HEAD") == 0){
    request->method = HttpMethods::HEAD;
  }else if(request_method.compare("POST") == 0){
    request->method = HttpMethods::POST;
  }else if(request_method.compare("PUT") == 0){
    request->method = HttpMethods::PUT;
  }else if(request_method.compare("DELETE") == 0){
    request->method = HttpMethods::DELETE;
  }else if(request_method.compare("CONNECT") == 0){
    request->method = HttpMethods::CONNECT;
  }else if(request_method.compare("OPTIONS") == 0){
    request->method = HttpMethods::OPTIONS;
  }else if(request_method.compare("TRACE") == 0){
    request->method = HttpMethods::TRACE;
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
        case HttpMethods::HEAD :
          ProcessHeadRequest(ssl_stream);
          break;
        case HttpMethods::DELETE :
          ProcessDeleteRequest(ssl_stream);
          break;
        case HttpMethods::OPTIONS :
          ProcessOptionsRequest(ssl_stream);
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
    m_RequestedResource = std::string("/index.html");
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

std::string HttpsService::GetCGIProgram(std::string resource_file)
{
  std::ifstream in(resource_file, std::ios::in);
  char data[4096];
  while(in.getline(data, 4096)){
    std::string str(data);
    std::size_t find = str.find("#!");
    if(find != std::string::npos){
      std::string program = str.substr(find + 2, str.length() - 2);
      program.erase(remove_if(program.begin(), program.end(), isspace), program.end());
      in.close();

      //return std::move(program);
            return program;
    }
  }
  in.close();
  return "";
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

           std::string program = GetCGIProgram(resource_file_path);

           //if program name is empty and requested source contain .py,
           //then set program to python3 by default
           if(program.empty() && boost::contains(m_RequestedResource, ".py")){
             program = "/usr/bin/python3";
           }

           if(program.empty() && boost::contains(m_RequestedResource, ".pl")){
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

//process head request by sending only headers not file
void HttpsService::ProcessHeadRequest(SSLStream ssl_stream)
{
  if(m_RequestedResource.empty()){
    m_ResponseStatusCode = 400;
    SendResponse(ssl_stream);
    return;
  }

  std::string resource_file_path = RESOURCE_DIR_PATH + m_RequestedResource;

  if(!boost::filesystem::exists(resource_file_path)){
    m_ResponseStatusCode = 404;
    SendResponse(ssl_stream);
    return;
  }

  std::ifstream resource_fstream(resource_file_path, std::ifstream::binary);

  if(!resource_fstream.is_open()){
    m_ResponseStatusCode = 500;
    return;
  }

  resource_fstream.seekg(0, std::ifstream::end);
  m_ResourceSizeInBytes = static_cast<std::size_t>(resource_fstream.tellg());

  SendResponse(ssl_stream);

}

//delete the requested resource
void HttpsService::ProcessDeleteRequest(SSLStream ssl_stream)
{
  if(m_RequestedResource.empty()){
    m_ResponseStatusCode = 400;
    SendResponse(ssl_stream);
    return;
  }

  std::string resource_file_path = RESOURCE_DIR_PATH + m_RequestedResource;

  if(!boost::filesystem::exists(resource_file_path)){
    m_ResponseStatusCode = 404;
    SendResponse(ssl_stream);
    return;
  }

  std::remove(resource_file_path.c_str());

  SendResponse(ssl_stream);

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

/***

main

***/
int main() //no args
{

   std::string pwd = get_current_dir_name(); //pwd

   RESOURCE_DIR_PATH = pwd + "/programs";
   CONF_FILE_PATH = pwd + "/esketit.conf";
   SSL_CERT_PATH = pwd + "/sslcert";

   std::cout << std::endl << pwd << std::endl;

        unsigned short port = 8080;

    /**********************************

    Do a few things to start i.e init();

    1. Port, if no port is given we should just have a default port
    2. Check for valid SSL certs ./sslcert/ x.key & x.cert if they are not found revert to http
    3. Set the non user configurable standard htdocs path - ./programs/
    4. Option to disable the server in preference for an alt stack
    5. Basic conf /etc/xxxx.conf to read from
    6. No args and no switches, conf file only or reverts to defaults

    Just menial stuff, we'll just create an functions.hpp and put it all in there.

    ***********************************/

    //1 do we disable this server?
    int ret = filechecks();

    if (ret == 99999) {

            exit(0); //999999 arbitary return code to exit

    } else {

        port = ret;  // 2. what port to use
        std::cout << "Port is " << port << std::endl;

    }
    //2.
    ret = is_htdocs_there();

    if (ret == 1) {}
    else {}

    //3.
    ret = is_SSL_there();

    if (ret == 1) { //start SSL version

    //try{

            HttpsServer https_server;

            https_server.Start(port);

            //keep alive server for 5 minutes, delete this and next line
            //if you want to keep it alive for infinite time
            std::this_thread::sleep_for(std::chrono::seconds(60 * 5));

            https_server.Stop();

     //}catch(boost::system::system_error &e){
     // std::cout<<"Error occured, Error code = "<<e.code()
     //        <<" Message: "<<e.what();
     //}


    } else { //start http version

        std::cerr << "SSL certificates not found";
        //future version todo revert to http
            return 1;

    }

 // std::string path(argv[1]);
 // if(path.at(path.length() - 1) == '/')
 //   path.pop_back();

  return 0;
}
