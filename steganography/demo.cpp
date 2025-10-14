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
        //current_time = current_time + ".png";
        //commented out above line to prevent issues with .wav carrier
        //use carrier's extension for output (png or wav etc.)
    std::string carrier_ext = ".png";
    size_t pos = carrier.rfind('.');
    if (pos != std::string::npos) carrier_ext = carrier.substr(pos);
    current_time = current_time + carrier_ext;

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