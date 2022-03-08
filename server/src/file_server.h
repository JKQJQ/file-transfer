#ifndef SERVER_H_
#define SERVER_H_


#include <netinet/in.h>
#include <iostream>
#include <cstdlib>
#include <cstring> // memset macro
#include <string>
#include <thread>
#include <fstream>
#include <filesystem>

namespace file_server {
    static const int RECV_BUFFER_SIZE = 64;
    static const int MAX_PENDING = 10;
    static const int SEND_BUFFER_SIZE = 1400;

    static const char PACKET_PART_SEP = ':';    // the sign used to separate parts in packets
    static const char CLIENT_REQ_HEADER = 'R';
    static const int PACKET_SIZE_MIN = 4;  // "B:filename:"
    static const char SERVER_BEG_HEADER = 'B';
    static const char SERVER_MID_HEADER = 'M';
    static const char SERVER_END_HEADER = 'E';
    static const int SERVER_HEADER_LEN = 3;

    class FileServer {
    private:
        in_port_t serverPort;
        int serverSock;             // socket descriptor for server
        sockaddr_in serverAddress;
        std::string basePath;       // the path where the file is in

        

        void initialize() {
            // create socket for incoming connections
            if((serverSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
                throw std::runtime_error("Create socket for incoming connections failed\n");
            }

            // construct local address struct 
            memset(&serverAddress, 0, sizeof(sockaddr_in));     // Zero out struct
            serverAddress.sin_family = AF_INET;                 // IPv4 family
            serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);  // any incoming interface
            serverAddress.sin_port = htons(serverPort);         // local port

            // bind to local address
            if(bind(serverSock, (sockaddr*) &serverAddress, sizeof(serverAddress)) < 0) {
                throw std::runtime_error("Failed to bind to port " + std::to_string(serverPort));
            }

            // make the socket listening incoming connections
            if(listen(serverSock, MAX_PENDING) < 0) {
                throw std::runtime_error("Failed to start listening");
            }

            std::cout << "Server started.\n";
        }

        struct requestHandler {
            int clientSocket;
            std::string basePath;
            requestHandler(int clientSocket, std::string basePath)
                :
                clientSocket(clientSocket),
                basePath(basePath)
                {}

            void operator()() {
                int nBytesReceived;
                char buffer[RECV_BUFFER_SIZE];
                for(;;) {
                    nBytesReceived = recv(clientSocket, buffer, RECV_BUFFER_SIZE, 0);

                    if(nBytesReceived <= 0) {
                        std::cout << "No bytes is received, waiting for the next request\n";
                        continue;
                    }

                    // start check the file name
                    if(nBytesReceived < PACKET_SIZE_MIN || 
                        buffer[0] != CLIENT_REQ_HEADER || 
                        buffer[1] != PACKET_PART_SEP) {
                        std::cout << "Unrecongnized request, waiting for the next request\n";
                        continue;
                    }

                    // buffer format: "R:filename:", filename starts at 2
                    std::string fileName;
                    {
                        int offset = 2;
                        for(; offset < nBytesReceived && buffer[offset] != PACKET_PART_SEP; ++offset) {
                            fileName.push_back(buffer[offset]);
                        }

                        if(offset == nBytesReceived) {
                            // reaches the end of buffer and : is not found
                            std::cout << "Request integrity check fails: end separator not found.\n";
                            continue;
                        }
                    }

                    // start sending file back to client
                    auto filePath = std::filesystem::path(basePath);
                    filePath += fileName;

                    // TODO: check if the file exists
                    std::ifstream inFileStream(filePath, std::ios::in | std::ios::binary);
                    inFileStream.seekg(0, inFileStream.end);
                    long nBytesFileSize = inFileStream.tellg();
                    
                    // start sending
                    // data format "HEADER:filename:payload", e.g., "B:file.bin:payload" 
                    // payload integerity does not matter, tcp guarantees it
                    char sBuffer[SEND_BUFFER_SIZE]; // buffer for sending
                    sBuffer[1] = PACKET_PART_SEP;
                    int fileNameBeginOffset = 2;
                    int payloadBeginOffset = fileNameBeginOffset + fileName.length() + 1;
                    int nBytesPayload = SEND_BUFFER_SIZE - payloadBeginOffset;
                    strcpy(sBuffer + fileNameBeginOffset, fileName.c_str());
                    sBuffer[payloadBeginOffset-1] = PACKET_PART_SEP;

                    // begin packet
                    inFileStream.seekg(0, inFileStream.beg);
                    sBuffer[0] = SERVER_BEG_HEADER;
                    long nBytesSentTotal = 0;
                    inFileStream.read(sBuffer + payloadBeginOffset, nBytesPayload);
                    nBytesSentTotal = send(clientSocket, sBuffer, SEND_BUFFER_SIZE, 0);
                
                    // middle packets
                    sBuffer[0] = SERVER_MID_HEADER;
                    while(nBytesSentTotal < nBytesFileSize) {
                        // reading the chunk
                        inFileStream.seekg(nBytesSentTotal);
                        inFileStream.read(sBuffer, nBytesPayload);

                        // send
                        int nBytesSent = send(clientSocket, sBuffer, SEND_BUFFER_SIZE, 0);
                        if(nBytesSent > payloadBeginOffset) nBytesSentTotal += nBytesSent - payloadBeginOffset;

                        std::cout << "total bytes sent: " << nBytesSent << std::endl;
                    }

                    // end packet
                    sBuffer[0] = SERVER_END_HEADER;
                    send(clientSocket, sBuffer, SEND_BUFFER_SIZE, 0);
                    
                    std::cout << "Finished sending file\n";
                    break;
                }
            }
        };
        

        void loop() {

            std::cout << "Started listening loop on port: " + std::to_string(serverPort) << std::endl;

            for(;;) {
                sockaddr clientAddress;
                socklen_t clientAddressLength = sizeof(clientAddress);

                int clientSock = accept(serverSock, &clientAddress, &clientAddressLength);

                if(clientSock < 0) {
                    throw std::runtime_error("accept failed");
                }

                std::thread t(requestHandler(clientSock, basePath));
            }
        }


    public:
        FileServer(in_port_t serverPort, std::string basePath) 
            :
            serverPort(serverPort),
            basePath(basePath)
        {
            initialize();
            loop();
        }

    };
}

#endif