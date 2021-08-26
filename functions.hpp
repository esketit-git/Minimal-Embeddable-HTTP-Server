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

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
//https://www.boost.org/doc/libs/develop/libs/beast/doc/html/beast/ref/boost__beast__http__verb.html
//https://www.boost.org/doc/libs/develop/libs/beast/doc/html/beast/ref/boost__beast__http__request.html
#include <boost/process/cmd.hpp>

#include <unistd.h>

//global paths
std::string RESOURCE_DIR_PATH; //pwd and /programs/
std::string CONF_FILE_PATH; //usually /etc/ .conf
std::string SSL_KEY_PATH; // pwd and sslcert/ssl.key
std::string SSL_CERT_PATH; //pwd and sslcert/ssk.cert


int filechecks() {

    unsigned short port = 8080; std::string line;
    //std::ifstream is RAII, i.e. no need to call close

/********************* Conf file */

    std::ifstream conf(CONF_FILE_PATH);

    if (conf.is_open())
    {

        while(getline(conf, line)){ //read the file # ignore comments
            line.erase(remove_if(line.begin(), line.end(), ::isspace),
                                 line.end());
            if(line[0] == '#' || line.empty())
                continue;
                    auto delimiterPos = line.find("=");
                    std::string name = line.substr(0, delimiterPos);
                    std::string value = line.substr(delimiterPos + 1);

                    //std::cout << name << " - " << value << '\n';

                int value_int = std::stoi(value); //check for disable

            if (name.compare("enable") == 0) {
                    if (value_int == 0) return(99999);  //99999 means disable server, alt stack
            } else if (name.compare("port") == 0) {
                    if (value_int > 0 && value_int < 65535) port = value_int; //or use default
            } else port = 8080; //revert to default port

        } //while

    } else { //the conf file is missing

        //std::cerr << std::endl << "Couldn't open config file for reading." << std::endl;
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
      std::ifstream sFile1 (SSL_CERT_PATH);
      std::ifstream sFile2 (SSL_KEY_PATH);

      if (sFile1.is_open() && sFile2.is_open())
      {

          return 1;  //proceed with SSL https


      } else {


          return 0; // revert  start HTTP

      }


  sFile1.close(); sFile2.close();

}


//Send the POST header to PHP-CGI , the same for both http and https

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

#endif
