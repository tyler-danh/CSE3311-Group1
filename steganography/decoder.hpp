#ifndef DECODER_H
#define DECODER_H

#include <iostream>
#include "handler.hpp"
#include <vector>

class Decoder{
    public:
        Decoder(std::string fileName);
        bool openEncodedFile();
        bool pngDecode(std::string newFile);
        bool jpegDecode(std::string newFile);
    private:
        std::vector<unsigned char> file_data, extracted_data;
        bool file_check = false;
        Handler encodedFile;
        std::string encoded_name;
        bool checksumCheck(uint16_t checksum);
};

#endif