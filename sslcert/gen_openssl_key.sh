# generate SSL certificate and private key
openssl req -newkey rsa:2048 -nodes -keyout esketit.key -x509 -days 365 -out esketit.cert
