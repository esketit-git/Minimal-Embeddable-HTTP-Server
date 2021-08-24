#include <iostream>
#include <fstream>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/process.hpp>
//https://www.boost.org/doc/libs/develop/libs/beast/doc/html/beast/ref/boost__beast__http__verb.html
//https://www.boost.org/doc/libs/develop/libs/beast/doc/html/beast/ref/boost__beast__http__request.html
#include <boost/asio.hpp>
#include <boost/process/cmd.hpp>

#include <unistd.h>

//#include <ctime>
//#include <string>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

std::string RESOURCE_DIRECTORY_PATH;

namespace my_program_state
{
    std::size_t
    request_count()
    {
        static std::size_t count = 0;
        return ++count;
    }

    std::time_t
    now()
    {
        return std::time(0);
    }
}

class http_connection : public std::enable_shared_from_this<http_connection>
{
public:
    http_connection(tcp::socket socket)
        : socket_(std::move(socket))
    {
    }

    // Initiate the asynchronous operations associated with the connection.
    void
    start()
    {
        read_request();
        check_deadline();
    }

private:
    // The socket for the currently connected client.
    tcp::socket socket_;

    // The buffer for performing reads.
    beast::flat_buffer buffer_{8192};

    // The request message.
    http::request<http::dynamic_body> request_;

    // The response message.
    http::response<http::dynamic_body> response_;

    // The timer for putting a deadline on connection processing.
    net::steady_timer deadline_{
        socket_.get_executor(), std::chrono::seconds(60)};

        std::string m_RequestedResource;
        std::size_t m_ResourceSizeInBytes;
        std::unique_ptr<char[]> m_ResourceBuffer;

    // Asynchronously receive a complete request message.
    void read_request()
    {
        auto self = shared_from_this();

        http::async_read(
            socket_,
            buffer_,
            request_,
            [self](beast::error_code ec,
                std::size_t bytes_transferred)
            {
                boost::ignore_unused(bytes_transferred);
                if(!ec)
                    self->process_request();
            });

    }

    // Determine what needs to be done with the request message.
    void process_request()
    {
        response_.version(request_.version());
        response_.keep_alive(false);

        switch(request_.method())
        {
        case http::verb::get:
            response_.result(http::status::ok);
            response_.set(http::field::server, "meHs");
            GET_response();
            break;
        case http::verb::post:
            response_.result(http::status::ok);
            response_.set(http::field::server, "meHs");
            POST_response();
            break;
        default:
            // We return responses indicating an error if
            // we do not recognize the request method.
            response_.result(http::status::bad_request);
            response_.set(http::field::content_type, "text/plain");
            beast::ostream(response_.body())
                << "Invalid request-method '"
                << std::string(request_.method_string())
                << "'";
            break;
        }

        write_response();
    }

    // Construct a response message based on the program state.
    void GET_response()
    {

           //the GET / POST is in case http::verb::post:
            std::string webpage = std::string(request_.target());
            //get response_.target (file to get) and read the file in from the hard disk
            //and then display it using response.body -> beast::ostream(response_.body()) <<
            std::string resource_file_path = RESOURCE_DIRECTORY_PATH + webpage;

            response_.set(http::field::content_type, "text/html");

        if(request_.target() == "/time")
        {
           // response_.set(http::field::content_type, "text/html");
            beast::ostream(response_.body())
                <<  "<h1>Current time</h1>\n"
                <<  "<p>The current time is "
                <<  my_program_state::now()
                <<  " seconds since the epoch.</p>\n";
        }

        else if (webpage.substr(webpage.find_last_of(".") + 1) == "php" ||
                 webpage.substr(webpage.find_last_of(".") + 1) == "py"  ||
                 webpage.substr(webpage.find_last_of(".") + 1) == "cgi" ||
                 webpage.substr(webpage.find_last_of(".") + 1) == "py")
        {

                 POST_response(); return;
        }

        else
        {

            std::ifstream ifs(resource_file_path);

            //check for file exist or send "file not found"
            struct stat buffer;

                if (stat (resource_file_path.c_str(), &buffer) == 0) {

                      if(S_ISREG(buffer.st_mode)) {

                        std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
                        beast::ostream(response_.body()) << content;

                     } else {

                            beast::ostream(response_.body()) << "Server is embedded - send correct full paths only.";
                     }

                } else {

                        beast::ostream(response_.body()) << "Server is embedded - send correct full paths only.";

                }

        }

    } // GET_response


    void POST_response()
    {

    std:: string cmd; std::string webpage;
    int x = 0;

                std::string query_string = boost::beast::buffers_to_string(request_.body().data());


        //the GET / POST is in case http::verb::post:
        webpage = std::string(request_.target());

        //get response_.target (file to get) and read the file in from the hard disk
        //and then display it using response.body -> beast::ostream(response_.body()) <<
        std::string resource_file_path = RESOURCE_DIRECTORY_PATH + webpage;

                response_.set(http::field::content_type, "text/html");

              //if resource end in php last 3 letters from socket
              if (webpage.substr(webpage.find_last_of(".") + 1) == "php") {


                    std::string request_body = boost::beast::buffers_to_string(request_.body().data());

                    cmd = gen_cgi_script(request_body, resource_file_path, "");


                    x = 1;


            } else if (webpage.substr(webpage.find_last_of(".") + 1) == "py") {

                //can't do POST CGI, need more work

                cmd = "/usr/bin/python " + resource_file_path;

                x = 1;

            } else if (webpage.substr(webpage.find_last_of(".") + 1) == "pl") {


                //needs more work

                cmd = "/usr/bin/perl " + resource_file_path;

                x = 1;

            }



       if (x == 1) {

                //a temporary file where output of an PHP interpretor will be stores
                std::string pwd = get_current_dir_name();
                std::string temp_output_file = pwd + std::string("/tempfile");
                //std::string cmd = RESOURCE_DIRECTORY_PATH + std::string("/cgi.sh");
                                    //std::string chmod_script = "chmod 777 " + temp_output_file;
                                    //int ret = system(chmod_script.c_str());

                        std::string command = cmd + " > " + temp_output_file;

                       // std::cerr << command;

                        int ret = system(command.c_str());

                //boost::process::system(cmd, boost::process::std_out);// > temp_output_file);

                //send output to client
                std::ifstream ifs(temp_output_file);

                //check for file exist or send "file not found"
                struct stat buffer;

                if (stat (resource_file_path.c_str(), &buffer) == 0) {



                    std::string content((std::istreambuf_iterator<char>(ifs)),
                                        (std::istreambuf_iterator<char>()));

                        beast::ostream(response_.body()) << content;

                    //remove tempoutputfile
                    std::remove(cmd.c_str());
                    std::remove(temp_output_file.c_str());

                } else {


                        beast::ostream(response_.body()) << "Server is embedded - send correct full paths only.";
                }

       } else {  /*unsupported post scripting language*/


                    beast::ostream(response_.body()) << "Scripting language unsupported.";

       }


    } //end post


    std::string gen_cgi_script(std::string request_data, std::string file_name, std::string query_string)
    {

        /*Generate this in a script to execute,
            requires post content and the file
            returns filename for system to execute

        #!/bin/sh
        REQUEST_DATA="var_1=val_1&var_2=val_2"
        export GATEWAY_INTERFACE="CGI/1.1"
        export SERVER_PROTOCOL="HTTP/1.1"
        export QUERY_STRING="test=querystring"
        export REDIRECT_STATUS="200"
        export SCRIPT_FILENAME="/test.php"
        export REQUEST_METHOD="POST"
        export CONTENT_LENGTH=${#REQUEST_DATA}
        export CONTENT_TYPE="application/x-www-form-urlencoded;charset=utf-8"
        echo $REQUEST_DATA | /usr/bin/php-cgi

        */

    std::ofstream shell_script; // Create and open a .sh file

    std::string pwd = get_current_dir_name();
    std::string cgi_script_path = pwd + "/cgi.sh";

                shell_script.open (cgi_script_path);
                shell_script << "#!/bin/sh\n";
                shell_script << "REQUEST_DATA=\"" + request_data + "\"\n";
                shell_script << "export GATEWAY_INTERFACE=\"CGI/1.1\"\n";
                shell_script << "export SERVER_PROTOCOL=\"HTTP/1.1\"\n";
                shell_script << "export QUERY_STRING=\"" + query_string + "\"\n";
                shell_script << "export REDIRECT_STATUS=\"200\"\n";
                shell_script << "export SCRIPT_FILENAME=\"" + file_name + "\"\n";
                shell_script << "export REQUEST_METHOD=\"POST\"\n";
                shell_script << "export CONTENT_LENGTH=${#REQUEST_DATA}\n";
                shell_script << "export CONTENT_TYPE=\"application/x-www-form-urlencoded;charset=utf-8\"\n";
                shell_script << "echo $REQUEST_DATA | /usr/bin/php-cgi";
                shell_script.close();

                    std::string chmod_script = "chmod 777 " + cgi_script_path;

                int ret = system(chmod_script.c_str());

    return cgi_script_path;

    }

    // Asynchronously transmit the response message.
    void write_response()
    {
        auto self = shared_from_this();

        response_.content_length(response_.body().size());

        http::async_write(
            socket_,
            response_,
            [self](beast::error_code ec, std::size_t)
            {
                self->socket_.shutdown(tcp::socket::shutdown_send, ec);
                self->deadline_.cancel();
            });
    }

    // Check whether we have spent enough time on this connection.
    void check_deadline()
    {
        auto self = shared_from_this();

        deadline_.async_wait(
            [self](beast::error_code ec)
            {
                if(!ec)
                {
                    // Close socket to cancel any outstanding operation.
                    self->socket_.close(ec);
                }
            });
    }
};

// "Loop" forever accepting new connections.
void http_server(tcp::acceptor& acceptor, tcp::socket& socket)
{
  acceptor.async_accept(socket,
      [&](beast::error_code ec)
      {
          if(!ec)
              std::make_shared<http_connection>(std::move(socket))->start();
          http_server(acceptor, socket);
      });
}

int main(int argc, char* argv[])
{

    std::string pwd = get_current_dir_name();

    printf("Current working dir: %s\n", get_current_dir_name());

   RESOURCE_DIRECTORY_PATH = pwd + "/programs";

    try
    {
        // Check command line arguments.
        if(argc != 3)
        {
            std::cerr << "Usage: " << argv[0] << " <address> <port>\n";
            std::cerr << "  For IPv4, try:\n";
            std::cerr << "    receiver 0.0.0.0 80\n";
            std::cerr << "  For IPv6, try:\n";
            std::cerr << "    receiver 0::0 80\n";
            return EXIT_FAILURE;
        }

        auto const address = net::ip::make_address(argv[1]);
        unsigned short port = static_cast<unsigned short>(std::atoi(argv[2]));

        net::io_context ioc{1};

        tcp::acceptor acceptor{ioc, {address, port}};
        tcp::socket socket{ioc};
        http_server(acceptor, socket);

        ioc.run();
    }
    catch(std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
