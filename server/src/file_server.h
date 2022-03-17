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
#include <queue>
#include "../../common/request_handler.h"


namespace fileserver {
    static const int MAX_PENDING = 10;
  
    class FileServer {
    private:
        in_port_t serverPort;
        int serverSock;             // socket descriptor for server
        sockaddr_in serverAddress;
        std::pair<int,int> downloadFileIdRange; // the files to download
        int nWorkers;
        int workerId;
        std::string suffix;
        std::string prefix;
        std::string uploadPath;
        std::string downloadPath;

        
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
            while(bind(serverSock, (sockaddr*) &serverAddress, sizeof(serverAddress)) < 0) {
                int t = 1;
                std::cout << "Server failed to bind to port " + std::to_string(serverPort) << ", sleeping for " << t << " seconds." << std::endl;
                sleep(t);
            }

            // make the socket listening incoming connections
            if(listen(serverSock, MAX_PENDING) < 0) {
                throw std::runtime_error("Failed to start listening");
            }

            std::cout << "Server started.\n";
        }
        

        void loop() {

            std::cout << "Started listening loop on port: " + std::to_string(serverPort) << std::endl;

            for(;;) {
                std::shared_ptr<sockaddr> clientAddress = std::make_shared<sockaddr>();
                socklen_t clientAddressLength = sizeof(*clientAddress.get());

                int clientSock = accept(serverSock, clientAddress.get(), &clientAddressLength);

                if(clientSock < 0) {
                    throw std::runtime_error("accept failed");
                }

                std::queue<std::string> taskQueue;

                for(int t = downloadFileIdRange.first + workerId; t < downloadFileIdRange.second; t += nWorkers) {
                    int i = t / 50 + 1;
                    int j = t % 50 + 1;
                    std::string fileName = prefix + std::to_string(i) + "_" + std::to_string(j) + suffix;
                    taskQueue.push(fileName);
                }

                std::thread t(requesthandler::requestHandler(clientSock, uploadPath, downloadPath, taskQueue));
                t.join();
            }
        }


    public:
        FileServer(in_port_t serverPort, std::string uploadPath, std::string downloadPath, 
            int nWorkers, int workerId, std::pair<int, int> downloadFileIdRange, 
            std::string prefix, std::string suffix) 
            :
            serverPort(serverPort),
            uploadPath(uploadPath),
            downloadPath(downloadPath),
            nWorkers(nWorkers),
            workerId(workerId),
            downloadFileIdRange(downloadFileIdRange),
            suffix(suffix),
            prefix(prefix)
        {
            initialize();
            loop();
        }

    };
}

#endif