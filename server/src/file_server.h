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
#include <unistd.h>
#include <sys/socket.h>
#include <memory>
#include <algorithm>

namespace fileserver {
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

                    if(buffer[0] == 'E') {
                          // shutdown(clientSocket, SHUT_RDWR);  // not necessary
                        
                        std::cout << "Client requested to end connection, closed connection with client\n";
                        close(clientSocket);
                        return;
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

                    std::cout << "client requested file: " << filePath << std::endl;

                    // TODO: check if the file exists
                    std::ifstream inFileStream(filePath, std::ios::in | std::ios::binary);
                    inFileStream.seekg(0, inFileStream.end);
                    long nBytesFileSize = inFileStream.tellg();
                    
                    // start sending
                    // data format "filesize:payload", e.g., "14999:payload" 
                    // payload integerity does not matter, tcp guarantees it
                    char sBuffer[SEND_BUFFER_SIZE]; // buffer for sending
                    std::string fileSizeStr = std::to_string(nBytesFileSize);
                    int payloadBeginOffset = fileSizeStr.length() + 1;
                    strcpy(sBuffer, fileSizeStr.c_str());
                    sBuffer[fileSizeStr.length()] = PACKET_PART_SEP;
                    int nBytesPayload = SEND_BUFFER_SIZE - payloadBeginOffset;
                    
                    // begin packet
                    inFileStream.seekg(0, inFileStream.beg);
                    long nBytesSentTotal = 0;
                    inFileStream.read(sBuffer + payloadBeginOffset, nBytesPayload);
                    nBytesSentTotal = send(clientSocket, sBuffer, SEND_BUFFER_SIZE, 0) - payloadBeginOffset;

                    std::cout << fileName << ": "<< nBytesSentTotal << " sent, totally " << nBytesFileSize << std::endl; ;

                    // middle packets
                    nBytesPayload = SEND_BUFFER_SIZE;
                    while(nBytesSentTotal < nBytesFileSize) {
                        // reading the chunk
                        inFileStream.seekg(nBytesSentTotal);
                        inFileStream.read(sBuffer, nBytesPayload);

                        // send
                        int nBytesSent = send(clientSocket, sBuffer, nBytesPayload, 0);
                        nBytesSentTotal += nBytesSent;

                        nBytesPayload = std::min(nBytesFileSize - nBytesSentTotal, (long)SEND_BUFFER_SIZE);

                        std::cout << fileName << ": "<< nBytesSentTotal << " sent, totally " << nBytesFileSize << std::endl; ;
                    }

                    // end packet
                    // sBuffer[0] = SERVER_END_HEADER;
                    // send(clientSocket, sBuffer, SEND_BUFFER_SIZE, 0);
                    
                    std::cout << "Finished sending file, waiting for client finish...\n";

                    nBytesReceived = recv(clientSocket, buffer, RECV_BUFFER_SIZE, 0);

                    if(nBytesReceived && buffer[0] == 'F') {
                        std::cout << "client finished receiving\n";
                    }
                }
            }
        };
        

        void loop() {

            std::cout << "Started listening loop on port: " + std::to_string(serverPort) << std::endl;

            for(;;) {
                std::shared_ptr<sockaddr> clientAddress = std::make_shared<sockaddr>();
                socklen_t clientAddressLength = sizeof(*clientAddress.get());

                int clientSock = accept(serverSock, clientAddress.get(), &clientAddressLength);

                if(clientSock < 0) {
                    throw std::runtime_error("accept failed");
                }

                std::thread t(requestHandler(clientSock, basePath));
                t.join();
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