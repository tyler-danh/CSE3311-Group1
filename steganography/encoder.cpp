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
    if (secret_check == false or carrier_check == false){
        if (secret_check == false and carrier_check == false){
            std::cerr << "Error: Failed to open secret file and carrier file" << std::endl;
            return false;
        }
        else if (secret_check == false){
            std::cerr << "Error: Failed to open secret file" << std::endl;
            return false;
        }
        else if (carrier_check == false){
            std::cerr << "Error: Failed to open carrier file" << std::endl;
            return false;
        }
    }
    return true;
}

bool Encoder::lsb(){
    if(secret_file.getFileSize() > carrier_file.getFileSize()){
        std::cout << "Error: Secret file is larger than Carrier file." << std::endl;
        return false;
    }
    std::cout << "lsb\n";
    return true;
}