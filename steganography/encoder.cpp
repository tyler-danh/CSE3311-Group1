#include <iostream>
#include <vector>
#include "encoder.hpp"
#include "handler.hpp"

Encoder::Encoder(std::string secret, std::string carrier)
//constructor has an init list that create Handler object to handle input files
    :   secret_file(secret),
        carrier_file(carrier)
{
    std::cout << "Encoder ready." << std::endl;
}
bool Encoder::lsb(){
    //open both files and get their data
    if(secret_file.getExt() == ".txt") {
        secret_data = secret_file.readFile();
        std::cout << "Read secret" << std::endl;
    }
    if(secret_file.getExt() == ".png"){
        secret_file.readPng();
        secret_data_unsigned = secret_file.getPixelData();
        std::cout << "Read secret" << std::endl;
    }
    if(carrier_file.getExt() == ".png"){
        carrier_data_unsigned = carrier_file.getPixelData();
        std::cout << "Read Carrier" << std::endl;
    }
    return true;
}