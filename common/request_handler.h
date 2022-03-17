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
        std::string fileNameDownloadInProgress;

        // other side status
        bool isOtherSideReadyToEndConnection;

        // this side status
        bool isThisSideReadyToEndConnection;
        
        requestHandler(int clientSocket, std::string uploadPath, std::string downloadPath, std::queue<std::string> taskQueue)
            :
            clientSocket(clientSocket),
            uploadPath(uploadPath),
            downloadPath(downloadPath),
            taskQueue(taskQueue),
            nBytesToDownload(-1),
            isOtherSideReadyToEndConnection(false),
            isThisSideReadyToEndConnection(false),
            nBytesUploaded(0)
            // isPendingUpload(true)
            {}

        void sendAllData(char* buffer) {
            int nBytesSentTotal = 0;

            while(nBytesSentTotal < PACKET_SIZE) {
                int nBytesSent = send(clientSocket, buffer + nBytesSentTotal, PACKET_SIZE - nBytesSentTotal, 0);
                nBytesSentTotal += nBytesSent;
            }
        }

        void receiveAllData(char* buffer) {
            int nBytesReceivedTotal = 0;

            while(nBytesReceivedTotal < PACKET_SIZE) {
                int nBytesReceived = recv(clientSocket, buffer+nBytesReceivedTotal, PACKET_SIZE - nBytesReceivedTotal, 0);
                nBytesReceivedTotal += nBytesReceived;
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
                if(!taskQueue.empty() && taskQueue.front() != fileNameDownloadInProgress) {
                    // send request
                    buffer[offset++] = REQUEST_PACKET_HEADER;
                    buffer[offset++] = PACKET_SEPARATOR;

                    while(!taskQueue.empty() && taskQueue.front() != fileNameDownloadInProgress) {
                        std::string fileName = taskQueue.front();

                        if(std::filesystem::exists(getDownloadPath(fileName + SUCCESS_FILE_EXTENSION))) {
                            std::cout << fileName << " already exists, skipping...\n" << std::endl;
                            taskQueue.pop();
                            continue;
                        }
                        
                        std::cout << "[request]Got fileName from taskQueue: " << fileName << std::endl;
                        strcpy(buffer+offset, fileName.c_str());
                        fileNameDownloadInProgress = fileName;
                        offset += fileName.length();
                        break;
                    }
                    buffer[offset++] = PACKET_SEPARATOR;
                }

                // send data or nop or end
                if(!fileNameUploadInProgress.empty()) {
                    
                    if(nBytesUploaded == 0) {
                        buffer[offset++] = DATA_BEG_PACKET_HEADER;
                        buffer[offset++] = PACKET_SEPARATOR;
                        std::string strSize = std::to_string(nBytesToUpload);
                        std::cout << "[send]nBytesToUpload: " << nBytesToUpload << std::endl;
                        strcpy(buffer + offset, strSize.c_str());
                        offset += strSize.length();
                    } else {
                        buffer[offset++] = DATA_MID_PACKET_HEADER;
                    }
                    buffer[offset++] = PACKET_SEPARATOR;

                    long packetRemainingSize = PACKET_SIZE - offset;
                    int nBytesToRead = std::min(packetRemainingSize, nBytesToUpload - nBytesUploaded);
                    
                    inUploadFileStream.read(buffer+offset, nBytesToRead);
                    // offset += nBytesToRead;
                    nBytesUploaded += nBytesToRead;

                    if(nBytesToUpload <= nBytesUploaded) fileNameUploadInProgress.clear();
                } else if(taskQueue.empty()) {
                    buffer[offset++] = END_PACKET_HEADER;
                    buffer[offset++] = PACKET_SEPARATOR;
                    isThisSideReadyToEndConnection = true;
                    std::cout << "[send] This side is ready to end connection\n";
                } else {
                    std::cout << "[send] fileNameUploadInProgress" << fileNameUploadInProgress << std::endl;
                    buffer[offset++] = NIL_PACKET_HEADER;
                    buffer[offset++] = PACKET_SEPARATOR;
                }

                sendAllData(buffer);                

                receiveAllData(buffer);

                // parse the packet
                offset = 0;
                char packetHeader = buffer[offset++];
                std::cout << "[receive] packet header: " << packetHeader << std::endl;
                if(packetHeader == REQUEST_PACKET_HEADER) {
                    fileNameUploadInProgress.clear();
                    nBytesUploaded = 0;
                    offset++;
                    while(offset < PACKET_SIZE && buffer[offset] != PACKET_SEPARATOR) {
                        fileNameUploadInProgress.push_back(buffer[offset++]);
                    }

                    std::cout << "[receive] expected file: " << fileNameUploadInProgress  << std::endl;

                    // open the file and count how many byte are there.
                    std::cout << "[receive] The success file exists: " << getUploadPath(fileNameUploadInProgress + SUCCESS_FILE_EXTENSION);
                       
                    if(std::filesystem::exists(getUploadPath(fileNameUploadInProgress + SUCCESS_FILE_EXTENSION))) {
                        // std::cout << "isPendingUpload is set to false\n";
                        // isPendingUpload = false;
                        updateUploadInfileStream();
                    }

                    packetHeader = buffer[++offset];
                    offset += 1;
                }
                // offset++; // skip a separator

                std::cout << "[receive] second packetHeader: " << packetHeader << std::endl;
                
                switch(packetHeader) {
                    case DATA_BEG_PACKET_HEADER: {
                        std::string strSize;
                        offset++;
                        while(offset < PACKET_SIZE && buffer[offset] != PACKET_SEPARATOR) {
                            strSize.push_back(buffer[offset++]);
                        }
                        std::cout << "[receive] strSize: " << strSize << std::endl;
                        nBytesToDownload = std::stol(strSize);
                        std::cout << "[receive][switch] new nBytesToDownload: " << nBytesToDownload << std::endl;
                    }
                    case DATA_MID_PACKET_HEADER: 
                        std::copy(buffer + offset + 1, buffer + PACKET_SIZE, std::back_inserter(downloadBuffer));
                        break;
                    case NIL_PACKET_HEADER: 
                        break;
                    case END_PACKET_HEADER:
                        isOtherSideReadyToEndConnection = true;
                        std::cout << "[receive] Other side is ready to end connection.\n";
                        break;
                }

                if(isThisSideReadyToEndConnection && isOtherSideReadyToEndConnection) break;

                std::cout << "[receive] "<< downloadBuffer.size() << " bytes downloaded, expected " << nBytesToDownload << " bytes.\n";

                if(downloadBuffer.size() >= nBytesToDownload) {
                    std::string downloadFile = taskQueue.front();

                    auto p = getDownloadPath(downloadFile);
                    std::ofstream outFile(p, std::ios::out | std::ios::binary);
                    outFile.write(reinterpret_cast<const char*>(downloadBuffer.c_str()), nBytesToDownload);

                    // mark as successfully downloaded in the directory
                    auto pSuccess = getDownloadPath(downloadFile + SUCCESS_FILE_EXTENSION);
                    std::ofstream successOutFile(pSuccess, std::ios::out | std::ios::binary);
                    successOutFile.write(SUCCESS_FILE_EXTENSION.c_str(), SUCCESS_FILE_EXTENSION.length());

                    std::cout << "[receive] File is downloaded and save to " << p << std::endl;

                    // reset download attributes
                    downloadBuffer.clear();
                    nBytesToDownload = -1;
                    taskQueue.pop();
                }
            }
            std::cout << "File transfer is finished.\n";
            close(clientSocket);
            std::cout << "Closed connection on both ends.\n";
        }
    };
}


#endif