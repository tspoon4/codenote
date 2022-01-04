openssl genrsa -out key.pem 4096
openssl req -new -sha256 -key key.pem -out csr.pem
openssl x509 -req -in csr.pem -signkey key.pem -out cert.pem -days 1825
