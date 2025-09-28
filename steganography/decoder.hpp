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
    private:
        std::vector<unsigned char> file_data, extracted_data;
        bool file_check;
        Handler encodedFile;
};

#endif