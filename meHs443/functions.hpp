#ifndef FUNCTIONS_HPP_INCLUDED
#define FUNCTIONS_HPP_INCLUDED

#include <iostream>
#include <thread>
#include <string>
#include <memory>
#include <atomic>
#include <fstream>
#include <chrono>
#include <ctime>

#include <algorithm>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <unordered_map>
#include <boost/process.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

std::string RESOURCE_DIR_PATH;
std::string CONF_FILE_PATH;
std::string SSL_CERT_PATH;

//global paths
//extern const std::string RESOURCE_DIRECTORY_PATH; // e.g var/www/html
//extern std::string RESOURCE_DIRECTORY_PATH;
//extern std::string CONF_DIRECTORY_PATH; // e.g etc
//extern std::string SSL_CERT_DIRECTORY_PATH; // e.g /etc/certs

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
  HEAD,
  POST,
  PUT,
  DELETE,
  CONNECT,
  OPTIONS,
  TRACE,
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


    const std::string m_DefaultIndexPage = "<html>\n<head>\n<title>HTTPS Web Server</title>\
        \n<style type=\"text/css\" media=\"screen\">\
          \nbody, html {padding: 3px 3px 3px 3px;background-color: #D8DBE2;\
            font-family: Verdana, sans-serif;font-size: 11pt;text-align: center;\
          }\ndiv.main_page{position: relative;display: table;width: 800px;margin-bottom: 3px;\
          margin-left: auto;margin-right: auto;padding: 0px 0px 0px 0px;border-width: 2px;\
          border-color: #DA2447;border-style: solid;background-color: #FFFFFF;text-align: center;\
          }\ndiv.page_header{height: 99px;width: 100%;background-color: #F5F6F7;}\
          \ndiv.page_header span{margin: 15px 0px 0px 50px;font-size: 180%;font-weight: bold;}\
          \ndiv.content_section_text{padding: 4px 8px 4px 8px;color: #000000;font-size: 100%;\
          }\ndiv.section_header{padding: 3px 6px 3px 6px;background-color: #8E9CB2;color: #FFFFFF;\
          font-weight: bold;font-size: 112%;text-align: center;}div.section_header_red {\
          background-color: #DA2447;}\n.floating_element{position: relative;float: center;}\
          </style>\n</head>\n<body><br/><div class=\"main_page\"><div class=\"page_header floating_element\">\
          <span class=\"floating_element\"><br/>HTTPS Web Server</span></div>\
          <div class=\"content_section floating_element\">\
          <div class=\"section_header section_header_red\"><div id=\"about\"></div>It works!</div>\
          <div class=\"content_section_text\"><br/><p>\
          This is the default welcome page used to test the correct operation of the HTTPS Web Server.\
          <br>If you can read this page, it means that the HTTPS Web server at this site is working properly.\
          <br>You should <b>replace this file</b> (located at\
          <tt>the provided path when server is started /index.html</tt>)\
          before continuing to operate your HTTPS server.\
          <br>Request the static HTML page with Server IP and port <b>1234</b>.\
          <br>Request the PHP script and get it's the interpreted contents.\
          </p><br><a style=\"text-decoration:none;\"\
          href=\"https://github.com/pritamzope/http_web_server/\">\
          https://github.com/pritamzope/http_web_server/</a><br><br>\
          </div></div></div>\n</body>\n</html>";


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



int filechecks() {

    unsigned short port = 8080; int value_int; std::string line;
    //std::ifstream is RAII, i.e. no need to call close

/********************* Conf file */

    std::ifstream conf ( CONF_FILE_PATH );

    if (conf.is_open())
    {

        while(getline(conf, line)){ //read the file # ignore comments
            line.erase(remove_if(line.begin(), line.end(), ::isspace),
                                 line.end());
            if(line[0] == '#' || line.empty())
                continue;
                    auto delimiterPos = line.find("=");
                    auto name = line.substr(0, delimiterPos);
                    auto value = line.substr(delimiterPos + 1);

                    //std::cout << name << " - " << value << '\n';

            value_int = std::stoi(value); //check for disable

            if (name.compare("enable") != 0 && value_int == 0)  {

                return(99999);  //99999 means disable server - do not run

            }

            //check port otherwise use default
            if (name.compare("port") && value_int > 0 && value_int < 65535) {

                port = value_int;

            } else {

                port = 8080; //revert to default port
            }

        } //while

    } else { //the conf file is missing

        std::cerr << std::endl << "Couldn't open config file for reading." << std::endl;
        //set the default port and start the server
        port = 8080; //revert to default port

    }


conf.close();


    return port;

}

/********************* Working Dir */

/*
 make sure the base system is there and complete
 check the entire base system make sure we are good to go
 wget anything that is missing
 */
bool is_htdocs_there() {

    std::ifstream wFile (RESOURCE_DIR_PATH);
    //std::ifstream wFile ("programs/index.php");

    if (wFile.is_open())
    {


        return 1;


    } else {

        std::cerr << std::endl << "htdocs not found" << std::endl;
        return 0;

    }

    wFile.close();

}

bool is_SSL_there() {

  /********************* SSL file */
      std::ifstream sFile1 (SSL_CERT_PATH + "/fullchain.pem");
      std::ifstream sFile2 (SSL_CERT_PATH + "/privkey.pem");

      if (sFile1.is_open() && sFile2.is_open())
      {

          return 1;  //proceed with SSL https


      } else {


          return 0; // revert  start HTTP

      }


  sFile1.close(); sFile2.close();

}



#endif
