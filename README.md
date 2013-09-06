## fortytwohundred
    
    Simple Reliable Transfer Protocol (SRTP)
    Reliable UDP integrated with a simple one-way client to server file transfer application
    
    To compile:
    $ make
    $ make test
    
    To run the FTP Server:
    $ ./ftserver [-d] port
    
    To run the FTP Client:
    $ ./ftclient [-d] server_address port filename
    
    Optional [-d]:
    Flag to output debug messages
    
    Expected behaviour of Server:
    Server will wait indefinitely for incoming connections
    Bad requests to the server will be ignored
    Write requests will be ignored if the file already exists on the server
    If the file cannot be written on the server it will be ignored
    Requests that cannot be acked will be ignored
    Partial files will be discarded in the case of connection timeout
    Incomplete files will be discarded after a successful transfer
    The socket to the client will be closed upon successful file transfer
    
    Expected behaviour of Client:
    If the client cannot send a request to the server it will close
    If the connection to the server is lost the client will close
    If the client receives a file already exists response from the server it will close
    If the server rejects a request the client will close
    If a packet cannot be sent the client will close
    If the server closes the connection unexpectedly the client will close
    Client will exit with status 0 upon successful file transfer
    
    A transfer is assumed to be successful if the file created on the server does not differ to the one sent by the client

## License
GPLv3
