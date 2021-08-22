
# HTTP Web Server

--------------------------------------------------------------------------------

# What Is It?

An asynchronous tiny HTTP web server that supports HTTP web requests as well as 
PHP, CGI, Python, Perl etc. scripts execution on server sides.

# Compilation

### Dependency

  ##### GCC with C++11 compiler(g++)  &  Boost C++ Library

### Step 1: Getting the Source Code

You can download source by command:

    $ git clone https://github.com/pritamzope/http_web_server.git
    $ cd http_web_server

or you can get source via other ways you prefer at <https://github.com/pritamzope/http_web_server>

### Step 2: Compile and Run

Run command:

    $ make
    $ ./httpserver <resource-direcory-path>


## Example

Run server with directory path where all server files will be stored.
e.g.:

    $ ./httpserver /home/pritam/resource

Some testing examples are given in **test** directory.<br>
Copy the contents of **test** folder into /home/pritam/resource directory.<br>
Now open Web Browser and goto address **127.0.0.1:1234**.<br>
Server uses port 1234 by default which can be changed in source(main.cpp).<br>
It will show following server default page.

<img src="https://raw.githubusercontent.com/pritamzope/http_web_server/master/http_web_server/images/http_server_main_page.png" width="600" height="400"/>

That means server is started successfully and ready to serve clients.
Many clients are given in **test** directory such as static HTML pages,PHP, CGI, Python and Perl scripts.

Navigate your browser URL to **127.0.0.1:1234/test.html**, it will show following page for testing each web scripts.<br>
If server is running on another machine then use that machine's IP address.

<img src="https://raw.githubusercontent.com/pritamzope/http_web_server/master/http_web_server/images/http_server_test.png" width="400" height="500"/>


