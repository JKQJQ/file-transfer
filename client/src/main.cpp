#include <iostream>
#include <fstream>
#include <string>
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


int main() {

    fileclient::FileClient fileClient(40018, "10.216.68.189", 5000, "/data/team-10/remote/");

    std::string prefix = "trade";
    for(int i = 1; i <= 10; ++i) {
        fileClient.requestFile(prefix + std::to_string(i));
    }
    
    return 0;
}