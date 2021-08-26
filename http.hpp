#include "functions.hpp"


//namespace beast = boost::beast;         // from <boost/beast.hpp>
//namespace http = beast::http;           // from <boost/beast/http.hpp>
//namespace net = boost::asio;            // from <boost/asio.hpp>
//using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

//std::string RESOURCE_DIRECTORY_PATH;

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
    http_connection(boost::asio::ip::tcp::socket socket)
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
    boost::asio::ip::tcp::socket socket_;

    // The buffer for performing reads.
    boost::beast::flat_buffer buffer_{8192};

    // The request message.
    boost::beast::http::request<boost::beast::http::dynamic_body> request_;

    // The response message.
    boost::beast::http::response<boost::beast::http::dynamic_body> response_;

    // The timer for putting a deadline on connection processing.
    boost::asio::steady_timer deadline_{ socket_.get_executor(), std::chrono::seconds(60) };

        std::string m_RequestedResource;
        std::size_t m_ResourceSizeInBytes;
        std::unique_ptr<char[]> m_ResourceBuffer;

    // Asynchronously receive a complete request message.
    void read_request()
    {
        auto self = shared_from_this();

        boost::beast::http::async_read(
            socket_,
            buffer_,
            request_,
            [self](boost::beast::error_code ec,
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
        case boost::beast::http::verb::get:
            response_.result(boost::beast::http::status::ok);
            response_.set(boost::beast::http::field::server, "meHs");
            GET_response();
            break;
        case boost::beast::http::verb::post:
            response_.result(boost::beast::http::status::ok);
            response_.set(boost::beast::http::field::server, "meHs");
            POST_response();
            break;
        default:
            // We return responses indicating an error if
            // we do not recognize the request method.
            response_.result(boost::beast::http::status::bad_request);
            response_.set(boost::beast::http::field::content_type, "text/plain");
            boost::beast::ostream(response_.body())
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
        std:: string cmd;
    //its a GET request

        //the GET / POST is in case http::verb::post:
        std::string webpage = std::string(request_.target()); //file to get, get response_.target (file to get)

        //display it using response.body -> beast::ostream(response_.body())
        std::string resource_file_path = RESOURCE_DIR_PATH + webpage;

        response_.set(boost::beast::http::field::content_type, "text/html");

        if(request_.target() == "/time")
        {
           // response_.set(http::field::content_type, "text/html");
            boost::beast::ostream(response_.body())
                <<  "<h1>Current time</h1>\n"
                <<  "<p>The current time is "
                <<  my_program_state::now()
                <<  " seconds since the epoch.</p>\n";

        } else if (webpage.substr(webpage.find_last_of(".") + 1) == "php") {

                POST_response("GET");

        } else if (webpage.substr(webpage.find_last_of(".") + 1) == "py") {

                //python
                cmd = "/usr/bin/python " + resource_file_path;


        } else if (webpage.substr(webpage.find_last_of(".") + 1) == "pl") {

                //perl
                cmd = "/usr/bin/perl " + resource_file_path;


        } else if (webpage.substr(webpage.find_last_of(".") + 1) == "rb") {

                    //ruby

        } else {

            std::ifstream ifs(resource_file_path);

            //check for file exist or send "file not found"
            struct stat buffer;

                if (stat (resource_file_path.c_str(), &buffer) == 0) {

                      if(S_ISREG(buffer.st_mode)) {

                        std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
                        boost::beast::ostream(response_.body()) << content;

                     } else {

                            boost::beast::ostream(response_.body()) << "Server is embedded - send correct full paths only.";
                     }

                } else {

                        boost::beast::ostream(response_.body()) << "Server is embedded - send correct full paths only.";

                }

        }

    } // GET_response


    void POST_response(std::string method = "POST")
    {

    std:: string cmd; std::string webpage;
    int x = 0;

                std::string query_string = boost::beast::buffers_to_string(request_.body().data());


        //the GET / POST is in case http::verb::post:
        webpage = std::string(request_.target());

        //get response_.target (file to get) and read the file in from the hard disk
        //and then display it using response.body -> beast::ostream(response_.body()) <<
        std::string resource_file_path = RESOURCE_DIR_PATH + webpage;

                response_.set(boost::beast::http::field::content_type, "text/html");

              //if resource end in php last 3 letters from socket
              if (webpage.substr(webpage.find_last_of(".") + 1) == "php") {


                    std::string request_body = boost::beast::buffers_to_string(request_.body().data());

                    cmd = gen_cgi_script(request_body, resource_file_path, "", method);


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

                        boost::beast::ostream(response_.body()) << content;

                    //remove tempoutputfile
                    std::remove(cmd.c_str());
                    std::remove(temp_output_file.c_str());

                } else {


                        boost::beast::ostream(response_.body()) << "Server is embedded - send correct full paths only.";
                }

       } else {  /*unsupported post scripting language*/


                    boost::beast::ostream(response_.body()) << "Scripting language unsupported.";

       }


    } //end post


    // Asynchronously transmit the response message.
    void write_response()
    {
        auto self = shared_from_this();

        response_.content_length(response_.body().size());

        boost::beast::http::async_write(
            socket_,
            response_,
            [self](boost::beast::error_code ec, std::size_t)
            {
                self->socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
                self->deadline_.cancel();
            });
    }

    // Check whether we have spent enough time on this connection.
    void check_deadline()
    {
        auto self = shared_from_this();

        deadline_.async_wait(
            [self](boost::beast::error_code ec)
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
void http_server(boost::asio::ip::tcp::acceptor& acceptor, boost::asio::ip::tcp::socket& socket)
{
  acceptor.async_accept(socket,
      [&](boost::beast::error_code ec)
      {
          if(!ec)
                std::make_shared<http_connection>(std::move(socket))->start();
        http_server(acceptor, socket);
      });
}
