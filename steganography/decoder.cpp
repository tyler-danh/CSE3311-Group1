#include <iostream>
#include <vector>
#include "decoder.hpp"
#include "handler.hpp"

Decoder::Decoder(std::string fileName)
    :   encodedFile(fileName)
{
    std::cout << "Console: Initializing decoder..." << std::endl;
}

bool Decoder::openEncodedFile(){
    if (encodedFile.getExt() == ".txt") {
        file_check = encodedFile.readFile();
        file_data = encodedFile.getFileData();
    }
    else if (encodedFile.getExt() == ".png"){
        file_check = encodedFile.readPng();
        file_data = encodedFile.getPixelData();
    }
    if (file_check == false){
        std::cerr << "Error: Encoded file failed to open" << std::endl;
        return false;
    }
    return true;
}
bool Decoder::pngDecode(std::string newFile){
    std::streamsize offset = 0;
    uint8_t ext_len = 0;
    unsigned char extracted_byte = 0;
    int height, width = 0;

    //first get ext_len
    for (int i = 0; i < 8; ++i){
        unsigned char bit = file_data[offset] & 1;
        extracted_byte |= (bit << i);
        offset++;
    }
    ext_len = extracted_byte;
    if(ext_len == 0){
        std::cerr << "Error: Could not read extension length" << std::endl;
        return false;
    }
    extracted_byte = 0; //clear extracted byte

    //then extract file ext chars
    std::string file_ext = "";
    for (int i = 0; i < ext_len; ++i){
        for (int j = 0; j < 8; ++j){
            unsigned char bit = file_data[offset] & 1;
            extracted_byte |= (bit << j);
            offset++;
        }
        file_ext += static_cast<char>(extracted_byte);
        extracted_byte = 0;
    }
    std::cout << "Console: Succesfully extracted file extension: " << file_ext << std::endl;

    //extract image dimensions if extracted extension is a supported image
    if (file_ext == ".png" or file_ext == ".jpeg" or file_ext == ".jpg"){
        std::cout << "Console: Image detected. Extracting dimensions." << std::endl;
        unsigned char* height_bytes = reinterpret_cast<unsigned char*>(&height);
        for (int i = 0; i < sizeof(height); ++i){
            for (int j = 0; j < 8; ++j){
                unsigned char bit = file_data[offset] & 1;
                extracted_byte |= (bit << j);
                offset++;
            }
            height_bytes[i] = extracted_byte;
            extracted_byte = 0;
        }
        std::cout << "Console: Extracted height: " << height << std::endl;

        unsigned char* width_bytes = reinterpret_cast<unsigned char*>(&width);
        for (int i = 0; i < sizeof(width); ++i){
            for (int j = 0; j < 8; ++j){
                unsigned char bit = file_data[offset] & 1;
                extracted_byte |= (bit << j);
                offset++;
            }
            width_bytes[i] = extracted_byte;
            extracted_byte = 0;
        }
        std::cout << "Console: Extracted width: " << width << std::endl;
    }
    //extract data size
    uint32_t data_size = 0;
    unsigned char* size_bytes = reinterpret_cast<unsigned char*>(&data_size);
    //using data size of data_size for for loop
    for (int i = 0; i < sizeof(data_size); ++i){
        for (int j = 0; j < 8; ++j){
            unsigned char bit = file_data[offset] & 1;
            extracted_byte |= (bit << j);
            offset++;
        }
        size_bytes[i] = extracted_byte;
        extracted_byte = 0;
    }
    if (data_size == 0){
        std::cerr << "Error: Could not read data size" << std::endl;
        return false;
    }
    else{
        std::cout << "Console: Succesfully extracted data size: " << data_size << std::endl;
    }

    //extract file data based on data_size
    extracted_data.reserve(data_size);
    for (int i = 0; i < data_size; ++i){
        for (int j = 0; j < 8; ++j){
            unsigned char bit = file_data[offset] & 1;
            extracted_byte |= (bit << j);
            offset++;
        }
        extracted_data.push_back(extracted_byte);
        extracted_byte = 0;
    }
    //reusing the encodedFile obj
    if (file_ext == ".txt"){
        encodedFile.setBinaryFileData(extracted_data);
        newFile = newFile + file_ext;
        if(!encodedFile.writeFile(newFile)){
            std::cerr << "Error: Failed to write to " << newFile << std::endl;
            return false;
        }
    }
    else if (file_ext == ".png"){
        encodedFile.setPngPixelData(extracted_data);
        encodedFile.setImageDimensions(0, height);
        encodedFile.setImageDimensions(1, width);
        newFile = newFile + file_ext;
        if (!encodedFile.writePng(newFile)){
            std::cerr << "Error: Failed to write to " << newFile << std::endl;
            return false;
        }
    }

    return true;
}