#include <iostream>
#include <fstream>
#include <string>
#include "file_server.h"

struct Order {
    int order_id;
    int price;
    int volume;
    char combined;
};

// int stk_code = (combined >> 4);
// int direction = ((combined >> 3) & 1) 1 : -1;
// int type = (combined & 7);

void fileReadLocalTestUtil() {
    Order orders[2];

    std::ifstream myFile("/home/fenghe/contests/jk/client-server/common/data/data.bin", std::ios::in | std::ios::binary);

    myFile.read(reinterpret_cast<char*>(orders), sizeof(Order) * 2);

    for(int i = 0; i < 2; ++i) {
        std::cout << orders[i].order_id << " " << orders[i].price << " " << orders[i].volume << " " << orders[i].combined << std::endl;
    }
}


int main(int argc, char** argv) {
    // DON'T forget the last "/" !
    // Please use absolute path
    // command line:
    // file_server 40018 /data/team-10/remote/
    // if(argc != 3) {
    //     std::cout << "command line format: \n"
    //               << "file_server port file_directory\n";
    //     return 0;
    // }
    in_port_t port = std::stoi(std::string(argv[1]));
    std::string uploadPath = std::string(argv[2]);
    std::string downloadPath = std::string(argv[3]);
    int nWorkers = std::stoi(std::string(argv[4]));
    int workerId = std::stoi(std::string(argv[5]));
    int downloadFileIdStart = std::stoi(std::string(argv[6]));
    int downloadFileIdEnd = std::stoi(std::string(argv[7]));
    std::string prefix = std::string(argv[8]);
    std::string suffix = std::string(argv[9]);
    
    if(uploadPath.empty() || downloadPath.empty()) {
        std::cout << "uploadPath is empty\n";
        return 0;
    }
    if(uploadPath.back() != '/') uploadPath.push_back('/');
    if(downloadPath.back() != '/') downloadPath.push_back('/');

    fileserver::FileServer server(port, uploadPath, downloadPath, nWorkers, workerId, 
            {downloadFileIdStart, downloadFileIdEnd}, prefix, suffix);
    
    return 0;
}