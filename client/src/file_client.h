#ifndef FILE_CLIENT_
#define FILE_CLIENT_

#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <string>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <fstream>

namespace fileclient {
    static const int REQ_BUFFER_SIZE = 64;
    static const int RECV_BUFFER_SIZE = 1400;
    static const int FILE_NAME_START_OFFSET = 2;

    static const char PACKET_PART_SEP = ':';    // the sign used to separate parts in packets
    static const char CLIENT_REQ_HEADER = 'R';
    static const char SERVER_BEG_HEADER = 'B';
    static const char SERVER_MID_HEADER = 'M';
    static const char SERVER_END_HEADER = 'E';

    static const std::string SUCCESS_FILE_EXTENSION = ".success"; //  success file for marking success 
    

    class FileClient {
        int clientSock;
        in_port_t serverPort;
        in_port_t clientPort;
        std::string serverIP;
        std::string localPath;

        void initialize() {
            if((clientSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
                throw std::runtime_error("Create socket for incoming connections failed\n");
            }

            sockaddr_in serverAddress;  // server address struct
            memset(&serverAddress, 0, sizeof(serverAddress));
            serverAddress.sin_family = AF_INET;
            // convert address
            if((inet_pton(AF_INET, serverIP.c_str(), &serverAddress.sin_addr.s_addr)) <= 0) {
                throw std::runtime_error("inet_pton() failed with invalid IP string");
            }
            serverAddress.sin_port = htons(serverPort);
            
            // explicit bind for client
            sockaddr_in clientAddress;
            clientAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
            clientAddress.sin_family = AF_INET;
            clientAddress.sin_port = htons(clientPort);

            if(bind(clientSock, (sockaddr*) &clientAddress, sizeof(clientAddress)) < 0) {
                throw std::runtime_error("failed for client to bind to port: " + std::to_string(clientPort));
            }

            // Establish the connection to the server
            if(connect(clientSock, (sockaddr*)& serverAddress, sizeof(serverAddress)) != 0) {
                throw std::runtime_error("failed to connect to server.");
            }
        }

        void closeConnection() {
            close(clientSock);
            std::cout << "client closed connection to server\n";
        }

    public:

        FileClient(in_port_t serverPort, std::string serverIP, in_port_t clientPort, std::string localPath) 
            :
            serverPort(serverPort),
            serverIP(serverIP),
            clientPort(clientPort),
            localPath(localPath)
        {
            // init
            initialize();
            // the next step is to send request to server and start receive file
        }

        ~FileClient() {
            closeConnection();
        }

        void requestFile(std::string fileName) {
            // construct the request instruction to server
            char requestBuffer[REQ_BUFFER_SIZE];

            std::cout << "started downloading " << fileName << " frome server.\n";

            // packet format: `R:file_ab3f33.bin:`
            requestBuffer[0] = CLIENT_REQ_HEADER;
            requestBuffer[1] = PACKET_PART_SEP;
            int payloadStartOffset = FILE_NAME_START_OFFSET + fileName.length() + 1;
            strcpy(requestBuffer + FILE_NAME_START_OFFSET, fileName.c_str());
            requestBuffer[payloadStartOffset-1] = PACKET_PART_SEP;
            requestBuffer[payloadStartOffset] = 0;  // make a 0-terminated C-style string for debug
            std::cout << "request buffer: <" << requestBuffer << ">"<< std::endl;
            send(clientSock, requestBuffer, REQ_BUFFER_SIZE, 0);

            // try to receive file from server
            std::string payload;
            {
                char buffer[RECV_BUFFER_SIZE];
                for(;;) {
                    int nBytesReceived = recv(clientSock, buffer, RECV_BUFFER_SIZE, 0);

                    if(nBytesReceived <= payloadStartOffset || 
                        buffer[1] != PACKET_PART_SEP || 
                        buffer[payloadStartOffset-1] != PACKET_PART_SEP) {
                        std::cout << "Received wrong file chunk or incorrect packet format, skipped\n";
                        continue;
                    }
                    
                    std::string receivedFileName(buffer + FILE_NAME_START_OFFSET, fileName.length());

                    if(receivedFileName != fileName) {
                        std::cout << "Incorrect file chunk, skipped.\n";
                        continue;
                    }

                    switch(buffer[0]) {
                        case SERVER_BEG_HEADER: payload.clear();
                        case SERVER_MID_HEADER: 
                            std::copy(buffer + payloadStartOffset, buffer + nBytesReceived, std::back_inserter(payload));
                            break;
                        case SERVER_END_HEADER: {
                                // write to buffer
                                std::filesystem::path filePath = localPath;
                                filePath += std::filesystem::path(fileName);
                                std::ofstream outFile(filePath, std::ios::out | std::ios::binary);
                                outFile.write(reinterpret_cast<const char*>(payload.c_str()), payload.length());
                                payload.clear();

                                // mark as successful
                                auto successFlagPath = filePath;
                                successFlagPath += SUCCESS_FILE_EXTENSION;
                                std::ofstream outFileSuccess(successFlagPath, std::ios::out | std::ios::binary);
                                outFileSuccess.write("success", 8);

                                std::cout << "successfully downloaded file to " << filePath << std::endl;
                                return;
                            }
                        default:
                            std::cout << "unrecognized packet header, skipped.\n";
                            return;
                    }
                }
            }
            
        }
    };
}


#endif