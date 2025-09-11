#ifndef ENCODE_H
#define ENCODE_H

#include <string>
#include <vector>

class Encode{
    public:
        Encode(const std::string secret, const std::string carrier);
        void parseExt();
        std::string getExt(std::string item) const;
        
    private:
        std::string secret_name, carrier_name, secret_ext, carrier_ext;
        int carrier_width, carrier_height, carrier_size, secret_size;
        std::vector<char> secret_data, carrier_data;
};

#endif