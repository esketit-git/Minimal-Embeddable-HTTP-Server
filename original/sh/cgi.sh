#!/bin/sh
# These variables come from the CGI standard rfc3875
# https://datatracker.ietf.org/doc/html/rfc3875

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

#This script can be run by creating cgi.sh chmod 755, create
#a php file called test.php (SCRIPT_FILENAME) and

<?php

$v1 = $_POST['var_1'];
$v2 = $_POST['var_2'];

echo $v1;
echo "\n";
echo $v2;
echo "\n\n";

?>

The PHP-CGI executable gets the shell script and outputs the POST vars to the test.php.

For Windows, perhaps a batch file, version could be...


    set REDIRECT_STATUS=true
    set SCRIPT_FILENAME=d:\php\test.php
    set REQUEST_METHOD=POST
    set GATEWAY_INTERFACE=CGI/1.1
    set CONTENT_LENGTH=16
    set CONTENT_TYPE=application/x-www-form-urlencoded
    set HTTP_COOKIE=PHPSESSID=vfg5csi76qpt3qlfml359ad210
    set QUERY_STRING=id=123
    echo test=hello world | d:\php\php-cgi.exe
    pause

With test.php being...

    setcookie('name','xxoo');
    echo"get:";
    print_r($_GET);
    echo"\r\npost:";
    print_r($_POST);
    echo"\r\ncookie:";
    print_r($_COOKIE);

Similar Linux Version

export REDIRECT_STATUS=true
export SCRIPT_FILENAME=/var/www/test.php 
export REQUEST_METHOD=POST 
export HTTP_COOKIE=PHPSESSID=vfg5csi76qpt3qlfml359ad210
