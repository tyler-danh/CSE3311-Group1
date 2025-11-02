#include <iostream>
#include <string>
#include <cstdio>
#include <png.h>
#include <jpeglib.h>
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
    image_height = png_get_image_height(png, png_info);
    image_width = png_get_image_width(png, png_info);
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

    png_read_update_info(png, png_info);

    //now read image data into image_pixel_data and close file
    int row_bytes = png_get_rowbytes(png, png_info);
    image_pixel_data.resize(row_bytes * image_height);

    std::vector<png_bytep> row_pointers(image_height);
    for (int i = 0; i < image_height; i++){
        row_pointers[i] = &image_pixel_data[i * row_bytes];
    }
    file_size = image_pixel_data.size();
    png_read_image(png, row_pointers.data());
    png_destroy_read_struct(&png, &png_info, NULL);
    fclose(image_file);
    return true;
}
bool Handler::readJpeg(){
    if(file_ext != ".jpeg" and file_ext != ".jpg"){
        std::cerr << "Error: File " << file_name << " is not a jpeg/jpg" << std::endl;
        return false;
    }
    //initialize jpeg decompression obj and error handling
    //these are from libjpeg.h
    struct jpeg_decompress_struct decompress_info;
    struct jpeg_error_mgr jpeg_err;

    decompress_info.err = jpeg_std_error(&jpeg_err);
    jpeg_create_decompress(&decompress_info);

    //open jpeg file
    FILE* image_file = fopen(file_name.c_str(), "rb");
    if(!image_file){
        std::cerr << "Error: Could not open " << file_name << std::endl;
        jpeg_destroy_decompress(&decompress_info);
        return false;
    }
    jpeg_stdio_src(&decompress_info, image_file);

    //read jpeg header to get image info
    (void) jpeg_read_header(&decompress_info, TRUE);
    //decompress image
    (void) jpeg_start_decompress(&decompress_info);

    image_height = decompress_info.output_height;
    image_width = decompress_info.output_height;
    int c_channels = decompress_info.output_components; //3 for rgb

    image_pixel_data.resize(image_height * image_width * c_channels);

    //read image row by row (scanline by scanline)
    while(decompress_info.output_scanline < decompress_info.image_height){
        unsigned char* row_ptr = &image_pixel_data[decompress_info.output_scanline * image_width * c_channels];
        jpeg_read_scanlines(&decompress_info, &row_ptr, 1);
    }

    //clean up structs
    (void) jpeg_finish_decompress(&decompress_info);
    jpeg_destroy_decompress(&decompress_info);
    fclose(image_file);
    return true;
}
//----------WRITING----------
bool Handler::writeFile(const std::string name){
    std::ofstream file(name, std::ios::binary);
    if(!file.is_open()){
        std::cerr << "Error: Could not open " << name << std::endl;
        return false;
    }

    file.write(reinterpret_cast<const char*>(binary_file_data.data()), binary_file_data.size());

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
    if (image_pixel_data.size() != (size_t)image_width * image_height * 4) {
        std::cerr << "CRITICAL ERROR: Data size does not match dimensions!" << std::endl;
        std::cerr << "Expected size: " << (size_t)image_width * image_height * 4 << std::endl;
        std::cerr << "Actual size:   " << image_pixel_data.size() << std::endl;
        png_destroy_write_struct(&png, &png_info);
        fclose(image_file);
        return false;
    }

    png_init_io(png, image_file);
    //write png headers
    png_set_IHDR(png, png_info, image_width, image_height, 8, PNG_COLOR_TYPE_RGBA,
        PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, png_info);

    //write image data and close file
    int row_bytes = image_width * 4;
    std::vector<png_bytep> row_pointers(image_height);
    for(int i = 0; i < image_height; ++i){
        row_pointers[i] = const_cast<png_bytep>(&image_pixel_data[i * row_bytes]);
    }
    png_write_image(png, row_pointers.data());
    png_write_end(png, NULL);

    png_destroy_write_struct(&png, &png_info);
    fclose(image_file);
    return true;
}
bool Handler::writeJpeg(const std::string name){
    // if(file_ext != ".jpeg" and file_ext != ".jpg"){
    //     std::cerr << "Error: File " << file_name << " is not a jpeg/jpg" << std::endl;
    //     return false;
    // }
    //init jpeg structs for compression
    struct jpeg_compress_struct compress_info;
    struct jpeg_error_mgr jpeg_err;

    compress_info.err = jpeg_std_error(&jpeg_err);
    jpeg_create_compress(&compress_info);

    //create output file
    FILE* image_file = fopen(name.c_str(), "wb");
    if(!image_file){
        std::cerr << "Error: Cannot write " << name << " to jpeg file." << std::endl;
        jpeg_destroy_compress(&compress_info);
        return false;
    }
    jpeg_stdio_dest(&compress_info, image_file);

    //set image properties
    compress_info.image_height = image_height;
    compress_info.image_width = image_width;
    compress_info.input_components = 3; //RGB
    compress_info.in_color_space = JCS_RGB;

    //set defaults and quality
    jpeg_set_defaults(&compress_info);
    jpeg_set_quality(&compress_info, 95, TRUE); //adjust quality here, KEEP CONSTANT, NOT ALLOW USER INPUT

    //compress image
    jpeg_start_compress(&compress_info, TRUE);
    //write pixel data row by row (scanline by scanline)
    while (compress_info.next_scanline < compress_info.image_height){
        const unsigned char* row_ptr = &image_pixel_data[compress_info.next_scanline * image_width * 3];
        jpeg_write_scanlines(&compress_info, const_cast<JSAMPROW*>(&row_ptr), 1);
    }

    //cleanup structs
    jpeg_finish_compress(&compress_info);
    jpeg_destroy_compress(&compress_info);
    fclose(image_file);

    return true;
}

//----------SETTERS----------//

void Handler::setPngPixelData(std::vector<unsigned char> pixel_data){
    image_pixel_data = pixel_data;
}
void Handler::setBinaryFileData(std::vector<unsigned char> file_data){
    binary_file_data = file_data;
}
void Handler::setImageDimensions(int selector, int dimension){
    if (selector == 0){image_height = dimension;}
    else{image_width = dimension;}
}

//----------GETTERS----------//

std::string Handler::getExt() const{
    return file_ext;
}
std::vector<unsigned char> Handler::getPixelData() const{
    return image_pixel_data;
}
std::vector<unsigned char> Handler::getFileData() const{
    return binary_file_data;
}
std::streamsize Handler::getFileSize() const{
    return file_size;
}
int Handler::getImageDimensions(int selector) const{
    if (selector == 0){return image_height;}
    else{return image_width;}
}

