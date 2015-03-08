Application Level File­Sharing­Protocol with support for download and upload for files and indexed searching, between a pair of devices.
We support both tcp and udp protocols for communication.
This application is for Linux/Mac Os based operating systems.

compilation: gcc ftp.c -lssl -lcrypto or simply 'run.sh'

run using ./a.out <server_port> <server_ip> <client_port> type<"tcp"/"udp">

The possible commands are:-
IndexGet --longlist
IndexGet --shortlist <time1> <time2>
IndexGet --regex <regex>

FileUpload <file>

FileDownload <file>

FileHash --verify <file>
FileHash --checkall

Approach:-
There is a fork within the code so that server and client are running simultaniously
Based on the protocol mentioned when the code is run(tcp/udp) Upload/Download takes place

Note: The udp connection protocol is highly unreliable and it is does not recommend file sharing via this protocol

The server has a file permission which will have either "allow" or "deny" in it which allow's or denies uploading of files
