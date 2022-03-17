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
#include "../../common/request_handler.h"


namespace fileclient {

    class FileClient {
        int clientSocket;
        in_port_t serverPort;
        in_port_t clientPort;
        std::string serverIP;
        std::string uploadPath;
        std::string downloadPath;
        sockaddr_in* serverAddress;
        sockaddr_in* clientAddress;

        void initialize() {
            if((clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
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
            
            while(bind(clientSocket, (sockaddr*) clientAddress, sizeof(*clientAddress)) < 0) {
                int t = 1;
                std::cout << "Client failed to bind to port " + std::to_string(serverPort) << ", sleeping for " << t << " seconds." << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(t));
            }

            // Establish the connection to the server
            if(connect(clientSocket, (sockaddr*) serverAddress, sizeof(*serverAddress)) != 0) {
                throw std::runtime_error("failed to connect to server, errno: " + std::to_string(errno));
            }
        }

        void closeConnection() {
            send(clientSocket, "E", 1, 0);
            close(clientSocket);
            delete serverAddress;
            delete clientAddress;
            std::cout << "client closed connection to server\n";
        }

    public:

        FileClient(in_port_t serverPort, std::string serverIP, in_port_t clientPort, std::string& uploadPath,
                    std::string& downloadPath, std::queue<std::string> taskQueue) 
            :
            serverPort(serverPort),
            serverIP(serverIP),
            clientPort(clientPort),
            uploadPath(uploadPath),
            downloadPath(downloadPath)
        {
            // init
            initialize();

            
            // the next step is to send request to server and start receive file
            requesthandler::requestHandler(clientSocket, uploadPath, downloadPath, taskQueue)();
        }

        ~FileClient() {
            closeConnection();
        }
    };
}


#endif