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
bool Encoder::openFiles(){
    //open both files and get their data
    //so far only supports .txt & .png
    if(secret_file.getExt() == ".txt") {
        secret_check = secret_file.readFile();
        secret_data = secret_file.getFileData();
    }
    else if(secret_file.getExt() == ".png"){
        secret_check = secret_file.readPng();
        secret_data_unsigned = secret_file.getPixelData();
    }
    if(carrier_file.getExt() == ".png"){
        carrier_check = carrier_file.readPng();
        carrier_data_unsigned = carrier_file.getPixelData();
    }
    if (secret_check == false){
        std::cout << "Failed to open secret file" << std::endl;
        return false;
    }
    if (carrier_check == false){
        std::cout << "Failed to open carrier file" << std::endl;
        return false;
    }
    return true;
}