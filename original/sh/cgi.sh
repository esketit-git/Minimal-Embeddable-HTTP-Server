#!/bin/sh
REQUEST_DATA="var_1=val_1&var_2=val_2"
export GATEWAY_INTERFACE="CGI/1.1"
export SERVER_PROTOCOL="HTTP/1.1"
export QUERY_STRING="test=querystring"
export REDIRECT_STATUS="200"
export SCRIPT_FILENAME="test.php"
export REQUEST_METHOD="POST"
export CONTENT_LENGTH=${#REQUEST_DATA}
export CONTENT_TYPE="application/x-www-form-urlencoded;charset=utf-8"
echo $REQUEST_DATA | /usr/bin/php-cgi
