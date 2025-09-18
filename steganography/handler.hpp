#ifndef HANDLER_H
#define HANDLER_H

#include <string>
#include <vector>
#include <fstream>

class Handler{
    public:
        Handler(const std::string file_name);
        void parseExt();
        std::vector<char> readFile(); //DO NOT USE THIS FOR IMAGES
        bool writeFile(const std::string name, const std::vector<char> binary);
        bool readPng();
        bool writePng(const std::string name);
        std::string getExt() const;
        std::vector<unsigned char> getPixelData() const;
        
    private:
        std::string file_name, file_ext;
        std::vector<char> binary_file_data; //NOT TO BE USED FOR IMAGES!!!
        std::vector<unsigned char> png_pixel_data;
        std::streamsize file_size;
        int png_width, png_height, jpeg_width, jpeg_height = 0;
};

#endif