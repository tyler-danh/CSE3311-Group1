#include <iostream>
#include <string>
#include <cstdio>
#include <png.h>
#include <zlib.h>
#include "handler.hpp"

Handler::Handler(std::string file_name){
    this->file_name = file_name;
    parseExt();
}
void Handler::parseExt(){
    //check file extentions of the file
    if(file_name.contains(".txt")){file_ext=".txt";}
    else if(file_name.contains(".png")){file_ext=".png";}
    else if(file_name.contains(".jpeg")){file_ext=".jpeg";}
    else if(file_name.contains(".jpg")){file_ext=".jpg";}
    else{file_ext = "INVALID";}
}
//----------READING-----------
bool Handler::readFile(){
    std::ifstream file(file_name, std::ios::binary | std::ios::ate);
    if(!file.is_open()){
        std::cerr << "Error: Could not open " << file_name << std::endl;
        return false;
    }

    file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    //this is a shortcut to get the filesize instead of incrementing a loop while reading
    //open and move pos to end, check pos for filesize, then move to beg

    binary_file_data.resize(file_size); //update buffer size based on file
    if(!file.read(reinterpret_cast<char*>(binary_file_data.data()), file_size)){
        std::cerr << "Error: Could not read " << file_name << std::endl;
        return false;
    }
    file.close();
    return true;
}
bool Handler::readPng(){
    if (file_ext != ".png"){
        std::cerr << "File " << file_name << " is not png" << std::endl;
        return false; 
    }
    FILE* image_file = fopen(file_name.c_str(), "rb"); //convert file_name to char
    if (!image_file){
        std::cerr << "Error: Could not open file " << file_name << std::endl;
        return false;
    }
    //init png structs
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png){
        std::cerr << "Error: libpng read struct failed to initialize" << std::endl;
        fclose(image_file);
        return false;
    }
    png_infop png_info = png_create_info_struct(png);
    if (!png_info){
        png_destroy_read_struct(&png, NULL, NULL);
        std::cerr << "Error: libpng read info struct failed to initialize" << std::endl;
        fclose(image_file);
        return false;
    }
    //libpng's try catch for struct initialization
    if (setjmp(png_jmpbuf(png))){
        png_destroy_read_struct(&png, &png_info, NULL);
        fclose(image_file);
        return false;
    }

    //read binary_file_data as png
    png_init_io(png, image_file); //usage is png_init_io(png_structrp png_ptr, FILE *fp)
    png_read_info(png, png_info);
    //read image info
    png_height = png_get_image_height(png, png_info);
    png_width = png_get_image_width(png, png_info);
    png_byte color_type = png_get_color_type(png, png_info);
    png_byte bit_depth = png_get_bit_depth(png, png_info);

    //making sure png's color palette is rgb
    if(bit_depth == 16) png_set_strip_16(png);
    if(color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png);
    if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_expand_gray_1_2_4_to_8(png);
    if(png_get_valid(png, png_info, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png);
    if(color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
    if(color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png);

    std::cout << std::to_string(color_type);

    png_read_update_info(png, png_info);

    //now read image data into png_pixel_data and close file
    int row_bytes = png_get_rowbytes(png, png_info);
    png_pixel_data.resize(row_bytes * png_height);

    std::vector<png_bytep> row_pointers(png_height);
    for (int i = 0; i < png_height; i++){
        row_pointers[i] = &png_pixel_data[i * row_bytes];
    }
    file_size = png_pixel_data.size();
    png_read_image(png, row_pointers.data());
    png_destroy_read_struct(&png, &png_info, NULL);
    fclose(image_file);
    return true;
}
//----------WRITING----------
bool Handler::writeFile(const std::string name, const std::vector<char> binary){
    std::ofstream file(name, std::ios::binary);
    if(!file.is_open()){
        std::cerr << "Error: Could not open " << name << std::endl;
        return false;
    }

    file.write(binary.data(), binary.size());

    if(!file.good()){
        std::cerr << "Error: Failed to write to " << name << std::endl;
        return false;
    }
    file.close();
    return true;
}
bool Handler::writePng(const std::string name){
    //this function assumes image is in simple RGBA format
    if (!name.contains(".png")){
        std::cerr << "Error: Cannot write " << name << " to png file" << std::endl;
        return false;
    }
    FILE* image_file = fopen(name.c_str(), "wb");
    if(!image_file){
        std::cerr << "Error: Could not open " << name << " for writing" << std::endl;
        return false;
    }

    //init libpng write structs
    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png){
        std::cerr << "Error: libpng write struct failed to initialize" << std::endl;
        fclose(image_file);
        return false;
    }
    png_infop png_info = png_create_info_struct(png);
    if (!png_info){
        png_destroy_write_struct(&png, NULL);
        std::cerr << "Error: libpng write info struct failed to initialize" << std::endl;
        fclose(image_file);
        return false;
    }
    png_set_compression_level(png, Z_BEST_COMPRESSION);
    //libpng's try catch for struct initialization
    if (setjmp(png_jmpbuf(png))){
        png_destroy_write_struct(&png, &png_info);
        fclose(image_file);
        return false;
    }
    //ensure image data aligns with image dimensions during read
    if (png_pixel_data.size() != (size_t)png_width * png_height * 4) {
        std::cerr << "CRITICAL ERROR: Data size does not match dimensions!" << std::endl;
        std::cerr << "Expected size: " << (size_t)png_width * png_height * 4 << std::endl;
        std::cerr << "Actual size:   " << png_pixel_data.size() << std::endl;
        png_destroy_write_struct(&png, &png_info);
        fclose(image_file);
        return false;
    }

    png_init_io(png, image_file);
    //write png headers
    png_set_IHDR(png, png_info, png_width, png_height, 8, PNG_COLOR_TYPE_RGBA,
        PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, png_info);

    //write image data and close file
    int row_bytes = png_width * 4;
    std::vector<png_bytep> row_pointers(png_height);
    for(int i = 0; i < png_height; ++i){
        row_pointers[i] = const_cast<png_bytep>(&png_pixel_data[i * row_bytes]);
    }
    png_write_image(png, row_pointers.data());
    png_write_end(png, NULL);

    png_destroy_write_struct(&png, &png_info);
    fclose(image_file);
    return true;
}
//----------SETTERS----------//
void Handler::setPngPixelData(std::vector<unsigned char> pixel_data){
    png_pixel_data = pixel_data;
}
//----------GETTERS----------//

std::string Handler::getExt() const{
    return file_ext;
}
std::vector<unsigned char> Handler::getPixelData() const{
    return png_pixel_data;
}
std::vector<unsigned char> Handler::getFileData() const{
    return binary_file_data;
}
std::streamsize Handler::getFileSize() const{
    return file_size;
}
