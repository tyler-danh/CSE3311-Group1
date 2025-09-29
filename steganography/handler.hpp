#ifndef HANDLER_H
#define HANDLER_H

#include <string>
#include <vector>
#include <fstream>

class Handler{
    public:
        Handler(const std::string file_name);
        void parseExt();
        bool readFile(); //DO NOT USE THIS FOR IMAGES
        bool writeFile(const std::string name);
        bool readPng();
        bool writePng(const std::string name);

        //setters
        void setPngPixelData(std::vector<unsigned char> pixel_data);
        void setBinaryFileData(std::vector<unsigned char> file_data);
        void setImageDimensions(int selector, int dimension);

        //getters
        std::string getExt() const;
        std::vector<unsigned char> getPixelData() const;
        std::vector<unsigned char> getFileData() const;
        std::streamsize getFileSize() const;
        int getImageDimensions(int selector) const;
        
    private:
        std::string file_name, file_ext;
        std::vector<unsigned char> binary_file_data; //NOT TO BE USED FOR IMAGES!!!
        std::vector<unsigned char> png_pixel_data;
        std::streamsize file_size;
        int image_width, image_height = 0;
};

#endif