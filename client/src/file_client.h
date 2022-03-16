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
#include <errno.h>
#include <memory>
#include <thread>
#include <chrono>


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
        sockaddr_in* serverAddress;
        sockaddr_in* clientAddress;

        void initialize() {
            if((clientSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
                throw std::runtime_error("Create socket for incoming connections failed\n");
            }

            serverAddress = new sockaddr_in;
            serverAddress->sin_family = AF_INET;
            // convert address
            serverAddress->sin_addr.s_addr = inet_addr(serverIP.c_str());
            // serverAddress.sin_addr.s_addr = inet_addr(serverIP.c_str());
            // if((inet_pton(AF_INET, serverIP.c_str(), &serverAddress.sin_addr.s_addr)) <= 0) {
            //     throw std::runtime_error("inet_pton() failed with invalid IP string");
            // }
            serverAddress->sin_port = htons(serverPort);
            
            // explicit bind for client
            clientAddress = new sockaddr_in;
            clientAddress->sin_addr.s_addr = INADDR_ANY;
            clientAddress->sin_family = AF_INET;
            clientAddress->sin_port = htons(clientPort);
            
            while(bind(clientSock, (sockaddr*) clientAddress, sizeof(*clientAddress)) < 0) {
                int t = 1;
                std::cout << "Client failed to bind to port " + std::to_string(serverPort) << ", sleeping for " << t << " seconds." << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(t));
            }

            // Establish the connection to the server
            if(connect(clientSock, (sockaddr*) serverAddress, sizeof(*serverAddress)) != 0) {
                throw std::runtime_error("failed to connect to server, errno: " + std::to_string(errno));
            }
        }

        void closeConnection() {
            send(clientSock, "E", 1, 0);
            close(clientSock);
            delete serverAddress;
            delete clientAddress;
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

        bool requestFile(std::string fileName) {
            // construct the request instruction to server
            char requestBuffer[REQ_BUFFER_SIZE];

            // TODO: if the file is already downloaded, skip it
            {
                std::filesystem::path successFilePath = localPath;
                successFilePath += fileName + SUCCESS_FILE_EXTENSION;
                std::cout << "Expected success flag: " << successFilePath << std::endl;
                if(std::filesystem::exists(successFilePath)) {
                    std::cout << "The file flag" << successFilePath << " already exists, skipped\n";
                    return true;
                }
            }

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
                int index = 0;
                char buffer[RECV_BUFFER_SIZE];

                // try to get file size
                std::string fileSizeStr;
                // buffer: 11223:xxcdxxxx
                while(true) {
                    int nBytesReceived = recv(clientSock, buffer, RECV_BUFFER_SIZE, 0);
                    std::cout << "file size nBytesReceived: " << nBytesReceived << std::endl;

                    if(nBytesReceived <= 0) {
                        int t = 1;
                        std::cout << "Trying to get file from server, but nothing received, returning and will request again...\n";
                        std::this_thread::sleep_for(std::chrono::seconds(t));
                        return false;
                    } else if(buffer[0] == '-') {
                        std::cout << "The file is not ready, download failed...\n";
                        return false;
                    }

                    int offset = 0;
                    for(; offset < nBytesReceived && buffer[offset] != PACKET_PART_SEP; ++offset) {
                        std::cout << buffer[offset];
                        fileSizeStr.push_back(buffer[offset]);
                    }
                    std::cout << std::endl;
                    std::cout << "offset: " << offset << std::endl;
                    if(offset != nBytesReceived) {
                        std::copy(buffer + offset + 1, buffer + nBytesReceived, std::back_inserter(payload));
                        break;
                    }
                }

                long fileSize = std::stol(fileSizeStr);

                for(int index = 0; payload.size() < fileSize; ++index) {
                    int nBytesReceived = recv(clientSock, buffer, RECV_BUFFER_SIZE, 0);

                    std::copy(buffer, buffer + nBytesReceived, std::back_inserter(payload));

                    std::cout << fileName << ": " << nBytesReceived << " " << payload.size() << ", expecting "<< fileSize << std::endl;
                }

                 // write to file
                std::filesystem::path filePath = localPath;
                filePath += std::filesystem::path(fileName);
                std::ofstream outFile(filePath, std::ios::out | std::ios::binary);
                outFile.write(reinterpret_cast<const char*>(payload.c_str()), fileSize);
                std::cout << "written " << fileSize << " bytes to " << filePath << std::endl; 

                // mark as successful
                auto successFlagPath = filePath;
                successFlagPath += SUCCESS_FILE_EXTENSION;
                std::ofstream outFileSuccess(successFlagPath, std::ios::out | std::ios::binary);
                outFileSuccess.write("success", 8);

                std::cout << "successfully downloaded file to " << filePath << std::endl;

                // requested to end the connection
                send(clientSock, "F", 1, 0);

                return true;
            }
            
        }
    };
}


#endif