#!/bin/bash

#TODO localhost resolution for cloud9
# if not defined IP
#   IP=127.0.0.1

IP=127.0.0.1
PORT=8080
SOURCE=src/file.txt

../ftpclient/ftpserver -d $PORT &

SERVER_PID=$!

echo "server running $SERVER_PID"

../ftpclient/ftpclient $IP $PORT $SOURCE &

CLIENT_PID=$!

echo "client running $CLIENT_PID"

wait $CLIENT_PID

echo "client stopped: $?"

kill -9 $SERVER_PID

wait $SERVER_PID

echo "server stopped: $?"

diff $SOURCE file.txt > /dev/null

DIFF_RES=$?

exit $DIFF_RES
