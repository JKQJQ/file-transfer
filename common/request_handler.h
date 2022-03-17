#ifndef REQUEST_HANDLER_
#define REQUEST_HANDLER_

#include <fstream>
#include <iostream>
#include <algorithm>
#include <queue>
#include <netinet/in.h>
#include <iostream>
#include <cstdlib>
#include <cstring> // memset macro
#include <string>
#include <thread>
#include <filesystem>
#include <unistd.h>
#include <sys/socket.h>
#include <memory>

namespace requesthandler {
    static const int PACKET_SIZE = 1400;

    static const char NIL_PACKET_HEADER = 'N';
    static const char REQUEST_PACKET_HEADER = 'R';
    static const char DATA_BEG_PACKET_HEADER = 'B';
    static const char DATA_MID_PACKET_HEADER = 'M';
    static const char END_PACKET_HEADER = 'E';
    static const char PACKET_SEPARATOR = ':';

    static const std::string SUCCESS_FILE_EXTENSION = ".success"; //  success file for marking success 
    struct requestHandler {
        int clientSocket;

        // upload
        std::string uploadPath;
        std::ifstream inUploadFileStream;
        long nBytesToUpload;
        std::string fileNameUploadInProgress;
        bool isPendingUpload;   // the file may not be available yet

        // download
        std::string downloadPath;
        std::queue<std::string> taskQueue;
        long nBytesToDownload;
        std::string downloadBuffer;

        // other side status
        bool isOtherSideReadyToEndConnection;
        
        requestHandler(int clientSocket, std::string uploadPath, std::string downloadPath, std::queue<std::string> taskQueue)
            :
            clientSocket(clientSocket),
            uploadPath(uploadPath),
            downloadPath(downloadPath),
            taskQueue(taskQueue),
            nBytesToDownload(-1),
            isOtherSideReadyToEndConnection(false),
            isPendingUpload(false)
            {}

        void sendAllData(char* buffer) {
            int nBytesSentTotal = 0;
            while(nBytesSentTotal < PACKET_SIZE) {
                int nBytesSent = send(clientSocket, buffer + nBytesSentTotal, PACKET_SIZE, 0);

                nBytesSentTotal += nBytesSent;

                std::cout << "Packet send size expected: " << PACKET_SIZE << ", " << nBytesSentTotal << " sent.\n";
            }
        }

        void receiveAllData(char* buffer) {
            // start receive
            int nBytesReceivedTotal = 0;

            while(nBytesReceivedTotal < PACKET_SIZE) {
                int nBytesReceived = recv(clientSocket, buffer+nBytesReceivedTotal, PACKET_SIZE, 0);

                nBytesReceivedTotal += nBytesReceived;
                std::cout << "Packet send size expected: " << PACKET_SIZE << ", " << nBytesReceivedTotal << " received.\n";
            }
        }

        inline std::filesystem::path getDownloadPath(std::string& fileName) {
            auto ret = std::filesystem::path(downloadPath);
            ret += std::filesystem::path(fileName);
            return ret;
        }

        inline std::filesystem::path getUploadPath(std::string& fileName) {
            auto ret = std::filesystem::path(uploadPath);
            ret += std::filesystem::path(fileName);
            return ret;
        }

        void updateUploadInfileStream() {
            inUploadFileStream = std::ifstream(getUploadPath(fileNameUploadInProgress), std::ios::binary | std::ios::in);
            inUploadFileStream.seekg(0, inUploadFileStream.end);
            nBytesToUpload = inUploadFileStream.tellg();
        }

        void operator()() {
            int nBytesReceived;
            char buffer[PACKET_SIZE];
            for(;;) {
                // construct and send the request or send data
                int offset = 0;
                if(nBytesToDownload == -1) {
                    // send request
                    buffer[offset++] = REQUEST_PACKET_HEADER;
                    buffer[offset++] = PACKET_SEPARATOR;

                    while(!taskQueue.empty()) {
                        auto& fileName = taskQueue.front();

                        if(!std::filesystem::exists(getDownloadPath(fileName))) {
                            std::cout << fileName << "already exists, skipping...\n" << std::endl;
                            taskQueue.pop();
                            continue;
                        }
                        
                        strcpy(buffer+offset, fileName.c_str());
                        offset += fileName.length();
                    }
                    buffer[offset++] = PACKET_SEPARATOR;
                }

                if(isPendingUpload && std::filesystem::exists(getUploadPath(fileNameUploadInProgress))) {
                    isPendingUpload = false;
                }

                // send data or nop
                if(!fileNameUploadInProgress.empty() && !isPendingUpload) {
                    long packetRemainingSize = PACKET_SIZE - offset;
                    int nBytesToRead = std::min(packetRemainingSize, nBytesToUpload);
                    nBytesToUpload -= nBytesToRead;
                    
                    if(downloadBuffer.empty()) {
                        buffer[offset++] = DATA_BEG_PACKET_HEADER;
                    } else {
                        buffer[offset++] = DATA_MID_PACKET_HEADER;
                    }
                    buffer[offset++] = PACKET_SEPARATOR;
                    
                    inUploadFileStream.read(buffer+offset, nBytesToRead);
                    offset += nBytesToRead;

                    if(nBytesToUpload == 0) fileNameUploadInProgress.clear();
                } else {
                    buffer[offset++] = NIL_PACKET_HEADER;
                    buffer[offset++] = PACKET_SEPARATOR;
                }

                sendAllData(buffer);

                receiveAllData(buffer);

                // parse the packet
                offset = 0;
                char packetHeader = buffer[offset++];
                if(packetHeader == REQUEST_PACKET_HEADER) {
                    fileNameUploadInProgress.clear();
                    while(offset < PACKET_SIZE && buffer[offset] != PACKET_SEPARATOR) {
                        fileNameUploadInProgress.push_back(buffer[offset++]);
                    }

                    // open the file and count how many byte are there.
                    if(!std::filesystem::exists(getUploadPath(fileNameUploadInProgress))) {
                        isPendingUpload = true;
                    }

                    offset += 2;
                    packetHeader = buffer[offset];
                }
                offset++; // skip a separator
                
                switch(packetHeader) {
                    case DATA_BEG_PACKET_HEADER: {
                        std::string strSize;
                        while(offset < PACKET_SIZE && buffer[offset] != PACKET_SEPARATOR) {
                            strSize.push_back(buffer[offset++]);
                        }
                        nBytesToDownload = std::stol(strSize);
                        offset++; // skip a separator
                    }
                    case DATA_MID_PACKET_HEADER: 
                        std::copy(buffer + offset, buffer + PACKET_SIZE, std::back_inserter(downloadBuffer));
                        break;
                    case NIL_PACKET_HEADER: 
                        break;
                    case END_PACKET_HEADER:
                        isOtherSideReadyToEndConnection = true;
                        break;
                }

                if(downloadBuffer.size() >= nBytesToDownload) {
                    std::string downloadFile = taskQueue.front();
                    taskQueue.pop();

                    std::ofstream outFile(getDownloadPath(downloadFile), std::ios::out | std::ios::binary);
                    outFile.write(reinterpret_cast<const char*>(downloadBuffer.c_str()), nBytesToDownload);

                    downloadBuffer.clear();
                    nBytesToDownload = -1;
                }
            }

            buffer[0] = END_PACKET_HEADER;
            buffer[0] = PACKET_SEPARATOR;
            sendAllData(buffer);
            buffer[0] = PACKET_SEPARATOR;

            while(buffer[0] != END_PACKET_HEADER && !isOtherSideReadyToEndConnection) {
                receiveAllData(buffer);
            }
        }
    };
}


#endif