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
    if(argc != 3) {
        std::cout << "command line format: \n"
                  << "file_server port file_directory\n";
        return 0;
    }
    std::string portStr = std::string(argv[1]);
    std::string localPath = std::string(argv[2]);

    fileserver::FileServer server(std::stoi(portStr), localPath);
    
    return 0;
}