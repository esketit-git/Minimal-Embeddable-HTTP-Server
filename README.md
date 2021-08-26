# minimal embeddable HTTP server (meHs)
A minimal HTTP / HTTPS Server capable of scripting for embedding. 

We are using Code::Blocks on Linux.

Based on https://github.com/pritamzope/http_web_server

Based on https://www.boost.org/doc/libs/develop/libs/beast/example/http/server/small/http_server_small.cpp

Project -> Build Options -> Links Settings add: boost_system boost_filesystem pthread crypto ssl

The original sources contained support for HEAD, DELETE and OPTIONS but they have been deleted. Only GET and POST requests are supported.
