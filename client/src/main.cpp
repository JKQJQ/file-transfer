#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>
#include "file_client.h"

struct Order {
    int order_id;
    int price;
    int volume;
    char combined;
};

// int stk_code = (combined >> 4);
// int direction = ((combined >> 3) & 1) 1 : -1;
// int type = (combined & 7);

void fileWriteLocalTestUtil() {
    Order orders[2] = {
        {1, 19900, 108, 0x73}, // 0x73(s): 01110011, stk_code = 7, direciton = 1, type = 4
        {2, 10901, 100, 0x6a}, // 0x64(j): 01101010, stk_code = 6, direction = 0, type = 2
    };

    std::ofstream myFile("/home/fenghe/contests/jk/client-server/common/data/data.bin", std::ios::out | std::ios::binary);
    
    myFile.write(reinterpret_cast<const char*>(orders), sizeof(Order) * 2);

}


int main(int argc, char **argv) {
    // command line example
    // file_client 6 0 40017 10.216.68.190 12345 /data/team-10/multiprocess-remote-test-limit/ 

    int nWorkers = std::stoi(std::string(argv[1]));
    int workerIndex = std::stoi(std::string(argv[2]));
    int serverPort = std::stoi(std::string(argv[3]));
    std::string serverAddress = std::string(argv[4]);
    int clientPort = std::stoi(std::string(argv[5]));
    std::string uploadPath = std::string(argv[6]);
    std::string downloadPath = std::string(argv[7]);
    int idRangeLower = std::stoi(std::string(argv[8]));
    int idRangeUpper = std::stoi(std::string(argv[9]));
    std::string prefix = std::string(argv[10]);
    std::string suffix = "";
    if(argc == 12) {
        suffix = std::string(argv[11]);
    }
    std::cout << "file suffix: " << suffix << std::endl;

    if(uploadPath.empty() || downloadPath.empty()) {
        std::cout << "localPath is empty\n";
        return 0;
    }
    if(uploadPath.back() != '/') uploadPath.push_back('/');
    if(downloadPath.back() != '/') downloadPath.push_back('/');
    
    auto start = std::chrono::steady_clock::now();

    std::queue<std::string> taskQueue;

    for(int t = idRangeLower + workerIndex; t < idRangeUpper; t += nWorkers) {
        int i = t / 50 + 1;
        int j = t % 50 + 1;
        std::string fileName = prefix + std::to_string(i) + "_" + std::to_string(j) + suffix;
        taskQueue.push(fileName);
    }
    
    fileclient::FileClient fileClient(serverPort, serverAddress, clientPort, uploadPath, downloadPath, taskQueue);

    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = end-start;
    std::cout << "elapsed time: " << elapsed_seconds.count() << "s\n";

    return 0;
}