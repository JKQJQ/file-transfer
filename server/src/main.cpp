#include <iostream>
#include <fstream>
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


int main() {
    
    FileServer server(5000, "/home/fenghe/contests/jk/client-server/common/data");
    
    return 0;
}