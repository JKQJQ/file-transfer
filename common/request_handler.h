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

    const std::string SUCCESS_FILE_EXTENSION = ".success"; //  success file for marking success 
    struct requestHandler {
        int clientSocket;

        // upload
        std::string uploadPath;
        std::ifstream inUploadFileStream;
        long nBytesToUpload;
        long nBytesUploaded;
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
            nBytesUploaded(0)
            // isPendingUpload(true)
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
                std::cout << "Packet receive size expected: " << PACKET_SIZE << ", " << nBytesReceivedTotal << " received.\n";
            }
        }

        inline std::filesystem::path getDownloadPath(const std::string& fileName) {
            auto ret = std::filesystem::path(downloadPath);
            ret += std::filesystem::path(fileName);
            std::cout << ret << std::endl;
            return ret;
        }

        inline std::filesystem::path getUploadPath(const std::string& fileName) {
            auto ret = std::filesystem::path(uploadPath);
            ret += std::filesystem::path(fileName);
            std::cout << ret << std::endl;
            return ret;
        }

        void updateUploadInfileStream() {
            inUploadFileStream = std::ifstream(getUploadPath(fileNameUploadInProgress), std::ios::binary | std::ios::in);
            inUploadFileStream.seekg(0, inUploadFileStream.end);
            nBytesToUpload = inUploadFileStream.tellg();
            std::cout << "update nBytesToUpload: " << nBytesToUpload << std::endl;
            inUploadFileStream.seekg(0, inUploadFileStream.beg);
        }

        void operator()() {
            char buffer[PACKET_SIZE];
            for(;;) {
                // construct and send the request or send data
                int offset = 0;
                if(nBytesToDownload == -1) {
                    // send request
                    buffer[offset++] = REQUEST_PACKET_HEADER;
                    buffer[offset++] = PACKET_SEPARATOR;

                    while(!taskQueue.empty()) {
                        std::string fileName = taskQueue.front();

                        if(std::filesystem::exists(getDownloadPath(fileName + SUCCESS_FILE_EXTENSION))) {
                            std::cout << fileName << " already exists, skipping...\n" << std::endl;
                            taskQueue.pop();
                            continue;
                        }
                        
                        std::cout << "Got fileName from taskQueue: " << fileName << std::endl;
                        strcpy(buffer+offset, fileName.c_str());
                        offset += fileName.length();
                        break;
                    }
                    buffer[offset++] = PACKET_SEPARATOR;
                }

                // send data or nop
                if(!fileNameUploadInProgress.empty()) {
                    
                    if(nBytesUploaded == 0) {
                        buffer[offset++] = DATA_BEG_PACKET_HEADER;
                        buffer[offset++] = PACKET_SEPARATOR;
                        std::string strSize = std::to_string(nBytesToUpload);
                        std::cout << "nBytesToUpload: " << nBytesToUpload << std::endl;
                        strcpy(buffer + offset, strSize.c_str());
                        offset += strSize.length();
                    } else {
                        buffer[offset++] = DATA_MID_PACKET_HEADER;
                    }
                    buffer[offset++] = PACKET_SEPARATOR;
                    buffer[offset] = 0;
                    std::cout << "temp buffer: " << buffer << std::endl;

                    long packetRemainingSize = PACKET_SIZE - offset;
                    int nBytesToRead = std::min(packetRemainingSize, nBytesToUpload - nBytesUploaded);
                    
                    inUploadFileStream.read(buffer+offset, nBytesToRead);
                    // offset += nBytesToRead;
                    nBytesUploaded += nBytesToRead;

                    if(nBytesToUpload <= nBytesUploaded) fileNameUploadInProgress.clear();
                } else {
                    std::cout << fileNameUploadInProgress << std::endl;
                    buffer[offset++] = NIL_PACKET_HEADER;
                    buffer[offset++] = PACKET_SEPARATOR;
                }

                sendAllData(buffer);                

                receiveAllData(buffer);

                // parse the packet
                offset = 0;
                char packetHeader = buffer[offset++];
                std::cout << "packet header: " << packetHeader << std::endl;
                if(packetHeader == REQUEST_PACKET_HEADER) {
                    fileNameUploadInProgress.clear();
                    nBytesUploaded = 0;
                    offset++;
                    while(offset < PACKET_SIZE && buffer[offset] != PACKET_SEPARATOR) {
                        fileNameUploadInProgress.push_back(buffer[offset++]);
                    }

                    std::cout << "expected file: " << fileNameUploadInProgress  << std::endl;

                    // open the file and count how many byte are there.
                    std::cout << "The success file exists: " << getUploadPath(fileNameUploadInProgress + SUCCESS_FILE_EXTENSION);
                       
                    if(std::filesystem::exists(getUploadPath(fileNameUploadInProgress + SUCCESS_FILE_EXTENSION))) {
                        // std::cout << "isPendingUpload is set to false\n";
                        // isPendingUpload = false;
                         updateUploadInfileStream();
                    }

                    packetHeader = buffer[++offset];
                }
                // offset++; // skip a separator

                std::cout << "second packetHeader: " << packetHeader << std::endl;
                
                switch(packetHeader) {
                    case DATA_BEG_PACKET_HEADER: {
                        std::string strSize;
                        offset += 2;
                        std::cout << "offset: " << offset << std::endl;
                        while(offset < PACKET_SIZE && buffer[offset] != PACKET_SEPARATOR) {
                            strSize.push_back(buffer[offset++]);
                        }
                        std::cout << "strSize: " << strSize << std::endl;
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

                std::cout << downloadBuffer.size() << " bytes downloaded, expected " << nBytesToDownload << " bytes.\n";

                if(downloadBuffer.size() >= nBytesToDownload) {
                    std::string downloadFile = taskQueue.front();
                    taskQueue.pop();

                    auto p = getDownloadPath(downloadFile);
                    std::ofstream outFile(p, std::ios::out | std::ios::binary);
                    outFile.write(reinterpret_cast<const char*>(downloadBuffer.c_str()), nBytesToDownload);

                    std::cout << "File is downloaded and save to " << p << std::endl;
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