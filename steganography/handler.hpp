#ifndef HANDLER_H
#define HANDLER_H

#include <string>
#include <vector>
#include <fstream>
#include <cstdint>
#include <jpeglib.h>

class Handler{
    public:
        Handler(const std::string file_name);
        void parseExt();
        bool readFile(); //DO NOT USE THIS FOR IMAGES
        bool writeFile(const std::string name);
        bool readPng();
        bool readWav();
        bool readJpeg();
        bool writePng(const std::string name);
        bool writeWav(const std::string name);
        bool writeJpeg(const std::string name);

        //setters
        void setPngPixelData(std::vector<unsigned char> pixel_data);
        void setWavSampleData(std::vector<unsigned char> sample_data);
        void setBinaryFileData(std::vector<unsigned char> file_data);
        void setImageDimensions(int selector, int dimension);

        //getters
        std::string getExt() const;
        std::vector<unsigned char> getPixelData() const;
        std::vector<unsigned char> getWavSampleData() const;
        std::vector<unsigned char> getFileData() const;
        std::streamsize getFileSize() const;
        int getImageDimensions(int selector) const;
        
    private:
        std::string file_name, file_ext;
        std::vector<unsigned char> binary_file_data; //NOT TO BE USED FOR IMAGES!!!
        std::vector<unsigned char> image_pixel_data;
        // WAV specific: offset into binary_file_data where sample bytes start and size
        std::streamsize wav_data_offset = 0;
        std::uint32_t wav_data_size = 0;
        std::streamsize file_size;
        int image_width, image_height = 0;
};

#endif