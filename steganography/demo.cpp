#include <iostream>
#include <string>
#include "handler.hpp"
#include "encoder.hpp"
#include <ctime>
#include <algorithm>

int main(){
    std::string secret, carrier;
    time_t timestamp = time(0);
    std::cout << "Enter Secret file: " << std::endl;
    std::cin >> secret;
    std::cout << "Enter Carrier file: " << std::endl;
    std::cin >> carrier;

    Encoder stega = Encoder(secret, carrier);
    if (stega.openFiles()){
        std::cout << "Console: Files successfully opened" << std::endl;
        std::string current_time = ctime(&timestamp);

        current_time.erase(std::remove(current_time.begin(), current_time.end(), ' '), current_time.end());
        current_time.erase(std::remove(current_time.begin(), current_time.end(), ':'), current_time.end());
        current_time.erase(std::remove(current_time.begin(), current_time.end(), '\n'), current_time.end());
        current_time = current_time + ".png";

        if (stega.lsb(current_time)){
            std::cout << "File " << current_time << " created succesfully" << std::endl;
        }
    }
    return 0;
}