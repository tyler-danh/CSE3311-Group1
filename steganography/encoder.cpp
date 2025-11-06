#include <iostream>
#include <vector>
#include "encoder.hpp"
#include <cstdint>
#include "handler.hpp"
#include <jpeglib.h>
#include <random>
#include <chrono>

Encoder::Encoder(std::string secret, std::string carrier)
//constructor has an init list that create Handler object to handle input files
    :   secret_file(secret),
        carrier_file(carrier)
{
    this->secret_name = secret;
    this->carrier_name = carrier;
    std::cout << "Console: Initializing Encoder..." << std::endl;
}
bool Encoder::openFiles(){
    //open both files and get their data
    //so far only supports .txt & .png
    if(secret_file.getExt() == ".txt") {
        secret_check = secret_file.readFile();
        secret_data = secret_file.getFileData();
    }
    else if(secret_file.getExt() == ".png"){
        secret_check = secret_file.readPng();
        secret_data = secret_file.getPixelData();
    }
    else if(secret_file.getExt() == ".jpeg" or secret_file.getExt() == ".jpg"){
        secret_check = secret_file.readJpeg();
        secret_data = secret_file.getPixelData();
    }
    //only checks png, jpeg files; other files with a valid secret will still pass
    if(carrier_file.getExt() == ".png"){
        carrier_check = carrier_file.readPng();
        carrier_data = carrier_file.getPixelData();
    }
    else if (carrier_file.getExt() == ".wav"){
        carrier_check = carrier_file.readWav();
        carrier_data = carrier_file.getWavSampleData();
    }
    else if(carrier_file.getExt() == ".jpeg" or carrier_file.getExt() == ".jpg"){
        // read JPEG pixel data so encoding methods that expect pixel bytes have data
        carrier_check = carrier_file.readJpeg();
        carrier_data = carrier_file.getPixelData();
    }
    if (secret_check == false or carrier_check == false){
        if (secret_check == false and carrier_check == false){
            std::cerr << "Error: Failed to open secret file and carrier file" << std::endl;
            return false;
        }
        else if (secret_check == false){
            std::cerr << "Error: Failed to open secret file" << std::endl;
            return false;
        }
        else if (carrier_check == false){
            std::cerr << "Error: Failed to open carrier file" << std::endl;
            return false;
        }
    }
    std::cout << "Console: Encoder ready" << std::endl;
    return true;
}

uint16_t Encoder::generateChecksum(){
    uint16_t checksum = 0;
    //initialize random number generator and seed with device's time since epoch
    //default_random_engine to choose different engines based on platform
    std::default_random_engine random_numbers(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<unsigned int> dist(1, INT16_MAX);
    //now set checksum to random number
    while(1){
        checksum = dist(random_numbers);
        //multiply by a constant for use in checking during decoding to ensure this is our checksum
        //data is very fragile anyways, we decided to keep it very simple
        checksum = checksum * 13;
        if (checksum % 13 == 0){
            break;
        }
    }
    std::cout << "Console: Checksum generated: " << checksum << std::endl;
    return checksum;
}

bool Encoder::pngLsb(std::string newFile){
    std::string secret_ext = secret_file.getExt();
    int secret_height = 0, secret_width = 0;
    if (secret_ext == ".png" or secret_ext == ".jpeg" or secret_ext == ".jpg"){
        secret_height = secret_file.getImageDimensions(0);
        secret_width = secret_file.getImageDimensions(1);
        std::cout << "Secret Height: " << secret_height << std::endl << "Secret Width " << secret_width << std::endl;
    } 
    std::streamsize secret_size = secret_file.getFileSize();
    //ext len + ext chars + datasize of the file size + actual file size (in bytes) -> times 8 bits
    std::streamsize required_bytes = (1 + static_cast<std::streamsize>(secret_ext.length()) + static_cast<std::streamsize>(secret_size)) * 8;
    // ensure carrier_data buffer has enough bytes to hold required bits
    if (required_bytes > static_cast<std::streamsize>(carrier_data.size())){
        std::cout << "Error: Secret file is too large." << std::endl;
        return false;
    }

    std::streamsize offset = 0;
    //encode the checksum first
    uint16_t checksum = generateChecksum();
    //then encode the bytes of the checksum
    unsigned char* checksum_bytes = reinterpret_cast<unsigned char*>(&checksum);
    for (size_t i = 0; i < sizeof(checksum); ++i){
        for (int j = 0; j < 8; ++j){
            unsigned char bit = (checksum_bytes[i] >> j) & 1;
            carrier_data[offset] &= 0xFE;
            carrier_data[offset] |= bit;
            offset++;
        }
    }

    //encode size of file ext
    //i use 'j' for for loops nested in another for loop - td
    uint8_t secret_ext_len = static_cast<uint8_t>(secret_ext.length());
    for (int i = 0; i < 8; ++i){
        unsigned char bit = (secret_ext_len >> i) & 1;
        carrier_data[offset] &= 0xFE;
        carrier_data[offset] |= bit;
        offset++;
    }
    //encode secret file's ext for when we decode
    for (char c : secret_ext){
        for (int j = 0; j < 8; ++j){
            c = static_cast<unsigned char>(c);
            unsigned char bit = (c >> j) & 1;
            carrier_data[offset] &= 0xFE;
            carrier_data[offset] |= bit;
            offset++;
        }
        
    }
    if (secret_ext == ".png" or secret_ext == ".jpeg" or secret_ext == ".jpg"){
        //encode the image's dimensions
        //height first
        //width next
        unsigned char* secret_height_bytes = reinterpret_cast<unsigned char*>(&secret_height);
        //changed std::streamsize to size_t for sizeof compatibility
        for (size_t i = 0; i < sizeof(secret_height); ++i){
            for (int j = 0; j < 8; ++j){
                unsigned char bit = (secret_height_bytes[i] >> j) & 1;
                carrier_data[offset] &= 0xFE;
                carrier_data[offset] |= bit;
                offset++;
            }
        }

        unsigned char* secret_width_bytes = reinterpret_cast<unsigned char*>(&secret_width);
        //changed std::streamsize to size_t for sizeof compatibility
        for (size_t i = 0; i < sizeof(secret_width); ++i){
            for (int j = 0; j < 8; ++j){
                unsigned char bit = (secret_width_bytes[i] >> j) & 1;
                carrier_data[offset] &= 0xFE;
                carrier_data[offset] |= bit;
                offset++;
            }
        }
    }
    //encode datasize of file next
    unsigned char* secret_size_bytes = reinterpret_cast<unsigned char*>(&secret_size);
    //changed std::streamsize to size_t for sizeof compatibility
    for (size_t i = 0; i < sizeof(secret_size); ++i){
        for (int j = 0; j < 8; ++j){
            unsigned char bit = (secret_size_bytes[i] >> j) & 1;
            carrier_data[offset] &= 0xFE;
            carrier_data[offset] |= bit;
            offset++;
        }
    }
    //now encode file data
    for (unsigned char file_byte : secret_data){
        for (int j = 0; j < 8; ++j){
            unsigned char bit = (file_byte >> j) & 1;
            carrier_data[offset] &= 0xFE;
            carrier_data[offset] |= bit;
            offset++;
        }
    }
    // update handler carrier file obj with encoded data and write new file
    if (carrier_file.getExt() == ".png"){
        carrier_file.setPngPixelData(carrier_data);
        carrier_file.writePng(newFile);
    }
    else if (carrier_file.getExt() == ".wav"){
        // carrier_data currently contains the raw file bytes read by readFile(); need to replace the wav samples
        carrier_file.setWavSampleData(carrier_data);
        carrier_file.writeWav(newFile);
    }
    return true;
}

bool Encoder::dctJpeg(std::string newFile){
    //intercepting compression process midway
    //file handling, reading and writing will have to be done here
    //setup decompression
    struct jpeg_decompress_struct decompress_info;
    struct jpeg_error_mgr jpeg_error;
    decompress_info.err = jpeg_std_error(&jpeg_error);

    if(carrier_check == false){
        std::cerr << "Error: Carrier file not valid" << std::endl;
        return false;
    }
    FILE* jpeg_file = fopen(carrier_name.c_str(), "rb");
    if(!jpeg_file){
        std::cerr << "Error: " << carrier_name << " failed to open" << std::endl;
        return false;
    }

    jpeg_create_decompress(&decompress_info);
    jpeg_stdio_src(&decompress_info, jpeg_file);
    jpeg_read_header(&decompress_info, TRUE);

    //read coefficients for DCT
    jvirt_barray_ptr* coefficients = jpeg_read_coefficients(&decompress_info);
    if (!coefficients){
        std::cerr << "Error: Failed to read JPEG coefficients." << std::endl;
        return false;
    }

    //setup compression
    struct jpeg_compress_struct compress_info;
    compress_info.err = jpeg_std_error(&jpeg_error);
    jpeg_create_compress(&compress_info);
    // if newFile already ends with .jpeg or .jpg, don't append
    if (!(newFile.size() >= 5 && (newFile.rfind(".jpeg") == newFile.size() - 5)) &&
        !(newFile.size() >= 4 && (newFile.rfind(".jpg") == newFile.size() - 4))) {
        // choose extension to append based on carrier's extension to preserve original style
        std::string jpeg_ext = ".jpeg";
        if (carrier_file.getExt() == ".jpg") jpeg_ext = ".jpg";
        newFile = newFile + jpeg_ext;
    }
    FILE* output_file = fopen(newFile.c_str(), "wb");
    if (!output_file){
        std::cerr << "Failed to open " << newFile << "for writing JPEG" << std::endl;
        return false;
    }
    jpeg_stdio_dest(&compress_info, output_file);
    jpeg_copy_critical_parameters(&decompress_info, &compress_info);

    //build the payload: checksum + ext + size + file_data
    //if its an image: checksum + ext + height + width + size + file_data
    std::vector<unsigned char> secret_payload;
    std::string secret_ext = secret_file.getExt();
    std::uint8_t ext_len = static_cast<uint8_t>(secret_ext.length());
    unsigned char* ext_len_bytes = reinterpret_cast<unsigned char*>(&ext_len);
    uint32_t secret_size = secret_data.size();
    unsigned char* size_bytes = reinterpret_cast<unsigned char*>(&secret_size);
    uint16_t checksum = generateChecksum();
    unsigned char* checksum_bytes = reinterpret_cast<unsigned char*>(&checksum);

    secret_payload.insert(secret_payload.end(), checksum_bytes, checksum_bytes + sizeof(checksum));
    secret_payload.insert(secret_payload.end(), ext_len_bytes, ext_len_bytes + sizeof(ext_len));
    secret_payload.insert(secret_payload.end(), secret_ext.begin(), secret_ext.end());
    //while its unlikely an image will fit in a jpeg, (even if its a png/jpeg) it will still be implemented
    if (secret_ext == ".png" or secret_ext == ".jpeg" or secret_ext == ".jpg"){
        int secret_height = secret_file.getImageDimensions(0);
        int secret_width = secret_file.getImageDimensions(1);
        std::cout << "Secret Height: " << secret_height << std::endl << "Secret Width " << secret_width << std::endl;
        unsigned char* secret_height_bytes = reinterpret_cast<unsigned char*>(&secret_height);
        unsigned char* secret_width_bytes = reinterpret_cast<unsigned char*>(&secret_width);

        secret_payload.insert(secret_payload.end(), secret_height_bytes, secret_height_bytes + sizeof(secret_height));
        secret_payload.insert(secret_payload.end(), secret_width_bytes, secret_width_bytes + sizeof(secret_width));
    } 
    secret_payload.insert(secret_payload.end(), size_bytes, size_bytes + sizeof(secret_size));
    secret_payload.insert(secret_payload.end(), secret_data.begin(), secret_data.end());

    //encoding logic
    size_t data_byte_index = 0;
    int data_bit_index = 0;
    bool finished_enc = false;

    //iterate through jpeg components (Y, Cb, Cr), blocks and coefficients
    for (int comp_i = 0; comp_i < decompress_info.num_components && !finished_enc; ++comp_i) {
        for (JDIMENSION block_y = 0; block_y < decompress_info.comp_info[comp_i].height_in_blocks && !finished_enc; ++block_y) {
            JBLOCKARRAY block_array = (decompress_info.mem->access_virt_barray)((j_common_ptr)&decompress_info, coefficients[comp_i], block_y, 1, FALSE);
            for (JDIMENSION block_x = 0; block_x < decompress_info.comp_info[comp_i].width_in_blocks && !finished_enc; ++block_x) {
                //each block has 64 coefficients
                for (int i = 0; i < DCTSIZE2; ++i) {
                    JCOEF* coef_ptr = &block_array[0][block_x][i];

                    //Steg Rule: only embed in coefficients that are not 0 or 1
                    if (*coef_ptr != 0 && *coef_ptr != 1) {
                        //get the current bit to hide
                        unsigned char current_byte = secret_payload[data_byte_index];
                        int bit_to_hide = (current_byte >> data_bit_index) & 1;
                        
                        //clear lsb and set it to our bit
                        *coef_ptr = (*coef_ptr & ~1) | bit_to_hide;

                        //move to the next bit/byte of secret data
                        data_bit_index++;
                        if (data_bit_index == 8) {
                            data_bit_index = 0;
                            data_byte_index++;
                            if (data_byte_index == secret_payload.size()) {
                                finished_enc = true;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    if(!finished_enc){
        std::cerr << "Error: Secret file is too large." << std::endl;
        jpeg_finish_compress(&compress_info);
        jpeg_destroy_compress(&compress_info);
        fclose(output_file);
        remove(newFile.c_str());
        jpeg_finish_decompress(&decompress_info);
        jpeg_destroy_decompress(&decompress_info);
        fclose(jpeg_file);
        return false;
    }
    //write modified coefficients to output_file
    jpeg_write_coefficients(&compress_info, coefficients);

    //cleanup
    jpeg_finish_compress(&compress_info);
    jpeg_destroy_compress(&compress_info);
    fclose(output_file);
    jpeg_finish_decompress(&decompress_info);
    jpeg_destroy_decompress(&decompress_info);
    fclose(jpeg_file);

    return true;
}

