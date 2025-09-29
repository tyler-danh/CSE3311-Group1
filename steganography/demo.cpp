#include <iostream>
#include <string>
#include "handler.hpp"
#include "encoder.hpp"
#include "decoder.hpp"
#include <ctime>
#include <algorithm>

int main(){
    std::string secret, carrier;
    time_t timestamp = time(0);
    std::cout << "Enter Secret file: " << std::endl;
    std::cin >> secret;
    std::cout << "Enter Carrier file: " << std::endl;
    std::cin >> carrier;
    std::string current_time = ctime(&timestamp);

    Encoder stega = Encoder(secret, carrier);
    if (stega.openFiles()){
        std::cout << "Console: Files successfully opened" << std::endl;
        current_time.erase(std::remove(current_time.begin(), current_time.end(), ' '), current_time.end());
        current_time.erase(std::remove(current_time.begin(), current_time.end(), ':'), current_time.end());
        current_time.erase(std::remove(current_time.begin(), current_time.end(), '\n'), current_time.end());
        current_time = current_time + ".png";

        if (stega.pngLsb(current_time)){
            std::cout << "File " << current_time << " created succesfully" << std::endl;
        }
    }
    
    Decoder saur = Decoder(current_time);
    if(saur.openEncodedFile()){
        if(saur.pngDecode("decode"))
        {
            std::cout << "decoded!" << std::endl;
        }
    }
    return 0;
}