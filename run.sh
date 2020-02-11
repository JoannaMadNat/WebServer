nc -C localhost 5000

echo -en "GET /tests/test1.txt HTTP/1.0\r\n\r\n" | nc localhost 5000

chrome http://127.0.0.1:5000/index.html

./httpdcrasher.py localhost 5000