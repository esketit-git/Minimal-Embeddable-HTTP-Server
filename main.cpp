#include "functions.hpp"
#include "http.hpp"
#include "https.hpp"

/***

main

***/
int main() //no args - conf or defaults
{

  unsigned short port = 8080;
//unsigned short port = static_cast<unsigned short>(port);

   std::string pwd = get_current_dir_name(); //pwd

   RESOURCE_DIR_PATH = pwd + "/programs";
   CONF_FILE_PATH = pwd + "/esketit.conf";
   SSL_CERT_PATH = pwd + "/sslcert/esketit.cert";
   SSL_KEY_PATH = pwd + "/sslcert/esketit.key";

   std::cout << std::endl << pwd << std::endl;


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

            std::cerr << "Server is disabled in conf file (enable = 0)" << std::endl;
            exit(0); //999999 arbitary return code to exit

    } else {

        port = ret;  // 2. what port to use

    }

    //2.
    if (is_htdocs_there()) {}
    else {}

    //3. are valid SSL certs there?
    if (is_SSL_there()) { //start SSL version

    //try{

            HttpsServer https_server;

            https_server.Start(port);

            std::cout << "Server started at address: 127.0.0.1 port " <<  port << std::endl;
            std::cout << "Goto https://127.0.0.1:" << port << std::endl;

            //keep alive server for 5 minutes, delete this and next line
            //if you want to keep it alive for infinite time
            std::this_thread::sleep_for(std::chrono::seconds(60 * 5));

            https_server.Stop();

     //}catch(boost::system::system_error &e){
     // std::cout<<"Error occured, Error code = "<<e.code()
     //        <<" Message: "<<e.what();
     //}


    } else { //start http version

        std::cerr << "SSL certificates not found. Revert to port 80" << std::endl;
        //future version todo revert to http

        std::cout << "Server started at address: 127.0.0.1 port " <<  port << std::endl;
        std::cout << "Goto http://127.0.0.1:" << port << std::endl;

 // std::string path(argv[1]);
 // if(path.at(path.length() - 1) == '/') path.pop_back();
//////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

    //try
    //{
          //  std::cerr << "  For IPv4, 0.0.0.0 80\n";
          //  std::cerr << "  For IPv6, 0::0 80\n";

        //auto const address = boost::asio::ip::make_address(argv[1]);
        //unsigned short port = static_cast<unsigned short>(std::atoi(argv[2]));
        //auto const address = boost::asio::ip::make_address("127.0.0.1");

        auto const address = boost::asio::ip::address_v4::any();

        boost::asio::io_context ioc{1};

        boost::asio::ip::tcp::acceptor acceptor{ioc, {address, port}};

        boost::asio::ip::tcp::socket socket{ioc};

        http_server(acceptor, socket);

        ioc.run();
    //}
    //catch(std::exception const& e)
    //{
    //    std::cerr << "Error: " << e.what() << std::endl;
    //    return EXIT_FAILURE;
    //}

    }

  return 0;
}
