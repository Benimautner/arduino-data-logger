openssl req -nodes -x509 -days 358000 -newkey rsa:2048 -keyout ca.key -out ca.crt -subj "/emailAddress=benimautner@gmail.com"

openssl req -nodes -newkey rsa:2048 -days 358000 -keyout server.key -out server.csr -subj "/emailAddress=benimautner@gmail.com"

openssl x509 -req -days 358000 -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out server.crt

cat server.key server.crt > server.pem

openssl req -nodes -newkey rsa:2048 -days 358000 -keyout client.key -out client.csr

openssl x509 -req -days 358000 -in client.csr -CA ca.crt -CAkey ca.key -CAserial ca.srl -out client.crt

cat client.key client.crt > client.pem
