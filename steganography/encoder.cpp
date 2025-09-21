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
        secret_data = secret_file.getPixelData();
    }
    //only checks png files, other files with a valid secret will still pass
    if(carrier_file.getExt() == ".png"){
        carrier_check = carrier_file.readPng();
        carrier_data = carrier_file.getPixelData();
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
    std::streamsize secret_size = secret_file.getFileSize();
    //datasize of the file size + actual file size * 8
    std::streamsize required_bytes = (sizeof(secret_size) + secret_file.getFileSize()) * 8;
    if(required_bytes > carrier_file.getFileSize()){
        std::cout << "Error: Secret file is too large." << std::endl;
        return false;
    }
    std::streamsize offset = 0;
    //encode datasize of file first
    unsigned char* secret_size_bytes = reinterpret_cast<unsigned char*>(&secret_size);
    for (std::streamsize i = 0; i < sizeof(secret_size); ++i){
        for (int j = 0; j < 8; ++j){
            unsigned char bit = (secret_size_bytes[i] >> j) & 1;
            carrier_data[offset] &= 0xFE;
            carrier_data[offset] |= bit;
            offset++;
        }
    }
    //now encode file data
    for (unsigned char file_byte : secret_data){
        for (int j = 0; j < 8; ++j){
            unsigned char bit = (file_byte >> j) & 1;
            carrier_data[offset] &= 0xFE;
            carrier_data[offset] |= bit;
            offset++;
        }
    }
    //now update Handler carrier_file obj with encoded pixel data and write new png
    carrier_file.setPngPixelData(carrier_data);
    carrier_file.writePng("dolphin2.png");
    return true;
}