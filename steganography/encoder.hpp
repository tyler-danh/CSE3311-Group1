#ifndef ENCODER_H
#define ENCODER_H

#include <iostream>
#include "handler.hpp"
#include <vector>

class Encoder{
    public:
        Encoder(std::string secret, std::string carrier);
        bool openFiles();
    private:
        std::vector<char> secret_data, carrier_data;
        std::vector<unsigned char> secret_data_unsigned, carrier_data_unsigned;
        bool secret_check, carrier_check;
        Handler secret_file, carrier_file;
};

#endif