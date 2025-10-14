#include <iostream>
#include <vector>
#include "encoder.hpp"
#include <cstdint>
#include "handler.hpp"

Encoder::Encoder(std::string secret, std::string carrier)
//constructor has an init list that create Handler object to handle input files
    :   secret_file(secret),
        carrier_file(carrier)
{
    std::cout << "Console: Initializing Encoder..." << std::endl;
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
    //only checks png files and wav files; other files with a valid secret will still pass
    if(carrier_file.getExt() == ".png"){
        carrier_check = carrier_file.readPng();
        carrier_data = carrier_file.getPixelData();
    }
    else if (carrier_file.getExt() == ".wav"){
        carrier_check = carrier_file.readWav();
        carrier_data = carrier_file.getWavSampleData();
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
    std::cout << "Console: Encoder ready" << std::endl;
    return true;
}

bool Encoder::pngLsb(std::string newFile){
    std::string secret_ext = secret_file.getExt();
    int secret_height, secret_width = 0;
    if (secret_ext == ".png" or secret_ext == ".jpeg" or secret_ext == ".jpg"){
        secret_height = secret_file.getImageDimensions(0);
        secret_width = secret_file.getImageDimensions(1);
        std::cout << "Secret Height: " << secret_height << std::endl << "Secret Width " << secret_width << std::endl;
    } 
    std::streamsize secret_size = secret_file.getFileSize();
    //ext len + ext chars + datasize of the file size + actual file size * 8
    std::streamsize required_bytes = (1 + secret_ext.length() + secret_size + secret_file.getFileSize()) * 8;
    if(required_bytes > carrier_file.getFileSize()){
        std::cout << "Error: Secret file is too large." << std::endl;
        return false;
    }

    std::streamsize offset = 0;
    //encode size of file ext first
    //i use 'j' for for loops nested in another for loop - td
    uint8_t secret_ext_len = static_cast<uint8_t>(secret_ext.length());
    for (int i = 0; i < 8; ++i){
        unsigned char bit = (secret_ext_len >> i) & 1;
        carrier_data[offset] &= 0xFE;
        carrier_data[offset] |= bit;
        offset++;
    }
    //encode secret file's ext for when we decode
    for (char c : secret_ext){
        for (int j = 0; j < 8; ++j){
            c = static_cast<unsigned char>(c);
            unsigned char bit = (c >> j) & 1;
            carrier_data[offset] &= 0xFE;
            carrier_data[offset] |= bit;
            offset++;
        }
        
    }
    if (secret_ext == ".png" or secret_ext == ".jpeg" or secret_ext == ".jpg"){
        //encode the image's dimensions
        //height first
        //width next
        unsigned char* secret_height_bytes = reinterpret_cast<unsigned char*>(&secret_height);
        //changed std::streamsize to size_t for sizeof compatibility
        for (size_t i = 0; i < sizeof(secret_height); ++i){
            for (int j = 0; j < 8; ++j){
                unsigned char bit = (secret_height_bytes[i] >> j) & 1;
                carrier_data[offset] &= 0xFE;
                carrier_data[offset] |= bit;
                offset++;
            }
        }

        unsigned char* secret_width_bytes = reinterpret_cast<unsigned char*>(&secret_width);
        //changed std::streamsize to size_t for sizeof compatibility
        for (size_t i = 0; i < sizeof(secret_width); ++i){
            for (int j = 0; j < 8; ++j){
                unsigned char bit = (secret_width_bytes[i] >> j) & 1;
                carrier_data[offset] &= 0xFE;
                carrier_data[offset] |= bit;
                offset++;
            }
        }
    }
    //encode datasize of file next
    unsigned char* secret_size_bytes = reinterpret_cast<unsigned char*>(&secret_size);
    //changed std::streamsize to size_t for sizeof compatibility
    for (size_t i = 0; i < sizeof(secret_size); ++i){
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
    // update handler carrier file obj with encoded data and write new file
    if (carrier_file.getExt() == ".png"){
        carrier_file.setPngPixelData(carrier_data);
        carrier_file.writePng(newFile);
    }
    else if (carrier_file.getExt() == ".wav"){
        // carrier_data currently contains the raw file bytes read by readFile(); need to replace the wav samples
        carrier_file.setWavSampleData(carrier_data);
        carrier_file.writeWav(newFile);
    }
    return true;
}