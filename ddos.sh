#!/bin/bash
port=$1

for number in {1..100}
do
echo -en "GET /tests/test1.txt HTTP/1.0\r\n\r\n" | nc localhost $port
done

