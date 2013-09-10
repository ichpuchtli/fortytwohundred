#!/bin/bash

rm -f ftpclient/file1.txt
rm -f ftpclient/file2.txt
rm -f ftpclient/file3.txt
rm -f ftpclient/file4.txt
rm -f ftpclient/file5.txt
rm -f ftpclient/file6.txt

./ftpclient/ftclient -d 127.0.0.1 8080 file1.txt  2> log1.txt &
PID1=$!

./ftpclient/ftclient -d 127.0.0.1 8080 file2.txt  2> log2.txt &
PID2=$!

./ftpclient/ftclient -d 127.0.0.1 8080 file3.txt  2> log3.txt &
PID3=$!

./ftpclient/ftclient -d 127.0.0.1 8080 file4.txt  2> log4.txt &
PID4=$!

./ftpclient/ftclient -d 127.0.0.1 8080 file5.txt  2> log5.txt &
PID5=$!

./ftpclient/ftclient -d 127.0.0.1 8080 file6.txt  2> log6.txt &
PID6=$!

wait $PID1
echo $?
wait $PID2
echo $?
wait $PID3
echo $?
wait $PID4
echo $?
wait $PID5
echo $?
wait $PID6
echo $?

echo 
echo 
echo 

sleep 5

diff -bq file1.txt ftpclient/file1.txt
echo $?
diff -bq file2.txt ftpclient/file2.txt
echo $?
diff -bq file3.txt ftpclient/file3.txt
echo $?
diff -bq file4.txt ftpclient/file4.txt
echo $?
diff -bq file5.txt ftpclient/file5.txt
echo $?
diff -bq file6.txt ftpclient/file6.txt
echo $?
