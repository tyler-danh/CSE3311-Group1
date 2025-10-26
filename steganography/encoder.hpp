#ifndef ENCODER_H
#define ENCODER_H

#include <iostream>
#include "handler.hpp"
#include <vector>
#include <string.h>

class Encoder{
    public:
        Encoder(std::string secret, std::string carrier);
        bool openFiles();
        bool pngLsb(std::string newFile);
        bool dctJpeg(std::string newFile);
    private:
        std::vector<unsigned char> secret_data, carrier_data;
        bool secret_check, carrier_check = false;
        std::string secret_name, carrier_name;
        Handler secret_file, carrier_file;
        uint16_t generateChecksum();
};

#endif