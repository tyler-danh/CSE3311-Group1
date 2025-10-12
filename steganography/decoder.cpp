#include <iostream>
#include <vector>
#include "decoder.hpp"
#include "handler.hpp"

Decoder::Decoder(std::string fileName)
    :   encodedFile(fileName)
{
    this->encoded_name = fileName;
    std::cout << "Console: Initializing decoder..." << std::endl;
}

bool Decoder::openEncodedFile(){
    if (encodedFile.getExt() == ".txt") {
        file_check = encodedFile.readFile();
        file_data = encodedFile.getFileData();
    }
    else if (encodedFile.getExt() == ".png"){
        file_check = encodedFile.readPng();
        file_data = encodedFile.getPixelData();
    }
    else if (encodedFile.getExt() == ".jpeg"){
        //must do actual checking in decoding method
        file_check = true;
    }
    if (file_check == false){
        std::cerr << "Error: Encoded file failed to open" << std::endl;
        return false;
    }
    return true;
}
bool Decoder::pngDecode(std::string newFile){
    std::streamsize offset = 0;
    uint8_t ext_len = 0;
    unsigned char extracted_byte = 0;
    int height, width = 0;

    //first get ext_len
    for (int i = 0; i < 8; ++i){
        unsigned char bit = file_data[offset] & 1;
        extracted_byte |= (bit << i);
        offset++;
    }
    ext_len = extracted_byte;
    if(ext_len == 0){
        std::cerr << "Error: Could not read extension length" << std::endl;
        return false;
    }
    extracted_byte = 0; //clear extracted byte

    //then extract file ext chars
    std::string file_ext = "";
    for (int i = 0; i < ext_len; ++i){
        for (int j = 0; j < 8; ++j){
            unsigned char bit = file_data[offset] & 1;
            extracted_byte |= (bit << j);
            offset++;
        }
        file_ext += static_cast<char>(extracted_byte);
        extracted_byte = 0;
    }
    std::cout << "Console: Succesfully extracted file extension: " << file_ext << std::endl;
    if (file_ext != ".txt" and file_ext != ".png" and file_ext != ".jpeg" and file_ext != ".jpg"){
        std::cerr << "CRITICAL ERROR: Extracted file extension is not valid. Aborting." << std::endl;
        return false;
    }

    //extract image dimensions if extracted extension is a supported image
    if (file_ext == ".png" or file_ext == ".jpeg" or file_ext == ".jpg"){
        std::cout << "Console: Image detected. Extracting dimensions." << std::endl;
        unsigned char* height_bytes = reinterpret_cast<unsigned char*>(&height);
        for (int i = 0; i < sizeof(height); ++i){
            for (int j = 0; j < 8; ++j){
                unsigned char bit = file_data[offset] & 1;
                extracted_byte |= (bit << j);
                offset++;
            }
            height_bytes[i] = extracted_byte;
            extracted_byte = 0;
        }
        std::cout << "Console: Extracted height: " << height << std::endl;

        unsigned char* width_bytes = reinterpret_cast<unsigned char*>(&width);
        for (int i = 0; i < sizeof(width); ++i){
            for (int j = 0; j < 8; ++j){
                unsigned char bit = file_data[offset] & 1;
                extracted_byte |= (bit << j);
                offset++;
            }
            width_bytes[i] = extracted_byte;
            extracted_byte = 0;
        }
        std::cout << "Console: Extracted width: " << width << std::endl;
    }
    //extract data size
    uint32_t data_size = 0;
    unsigned char* size_bytes = reinterpret_cast<unsigned char*>(&data_size);
    //using data size of data_size for for loop
    for (int i = 0; i < sizeof(data_size); ++i){
        for (int j = 0; j < 8; ++j){
            unsigned char bit = file_data[offset] & 1;
            extracted_byte |= (bit << j);
            offset++;
        }
        size_bytes[i] = extracted_byte;
        extracted_byte = 0;
    }
    if (data_size == 0){
        std::cerr << "Error: Could not read data size" << std::endl;
        return false;
    }
    else{
        std::cout << "Console: Succesfully extracted data size: " << data_size << std::endl;
    }

    //extract file data based on data_size
    extracted_data.reserve(data_size);
    for (int i = 0; i < data_size; ++i){
        for (int j = 0; j < 8; ++j){
            unsigned char bit = file_data[offset] & 1;
            extracted_byte |= (bit << j);
            offset++;
        }
        extracted_data.push_back(extracted_byte);
        extracted_byte = 0;
    }
    //reusing the encodedFile obj
    if (file_ext == ".txt"){
        encodedFile.setBinaryFileData(extracted_data);
        newFile = newFile + file_ext;
        if(!encodedFile.writeFile(newFile)){
            std::cerr << "Error: Failed to write to " << newFile << std::endl;
            return false;
        }
    }
    else if (file_ext == ".png"){
        encodedFile.setPngPixelData(extracted_data);
        encodedFile.setImageDimensions(0, height);
        encodedFile.setImageDimensions(1, width);
        newFile = newFile + file_ext;
        if (!encodedFile.writePng(newFile)){
            std::cerr << "Error: Failed to write to " << newFile << std::endl;
            return false;
        }
    }

    return true;
}
bool Decoder::jpegDecode(std::string newFile){
    //setup decompression
    struct jpeg_decompress_struct decompress_info;
    struct jpeg_error_mgr jpeg_error;
    decompress_info.err = jpeg_std_error(&jpeg_error);

    //open encoded file
    FILE* encoded = fopen(encoded_name.c_str(), "rb");
    if (!encoded){
        std::cerr << "Error: Failed to open " << encoded_name << std::endl;
        return false;
    }
    jpeg_create_decompress(&decompress_info);
    jpeg_stdio_src(&decompress_info, encoded);
    jpeg_read_header(&decompress_info, TRUE);

    //read DCT coefficients
    jvirt_barray_ptr* coeffcients = jpeg_read_coefficients(&decompress_info);
    if (!coeffcients){
        std::cerr << "Error: Failed to read " << encoded_name << " DCT coefficients." << std::endl;
        return false;
    }

    //setup for extraction
    unsigned char current_byte = 0;
    int bit_counter = 0;

    //things we need to extract
    uint8_t ext_len = 0;
    uint32_t file_size = 0;
    size_t total_size = 0;
    int height,width = 0;
    bool parsed = false; //once we have enough data mark as true to stop iterating

    //JSTEG extraction logic
    for (int comp_i = 0; comp_i < decompress_info.num_components; ++comp_i) {
        for (int block_y = 0; block_y < decompress_info.comp_info[comp_i].height_in_blocks; ++block_y) {
            JBLOCKARRAY block_array = (decompress_info.mem->access_virt_barray)((j_common_ptr)&decompress_info, coeffcients[comp_i], block_y, 1, FALSE);
            for (int block_x = 0; block_x < decompress_info.comp_info[comp_i].width_in_blocks; ++block_x) {
                for (int i = 0; i < DCTSIZE2; ++i) {
                    JCOEF coef_val = block_array[0][block_x][i];

                    //JSTEG Rule: Only extract from coefficients that are not 0 or 1
                    if (coef_val != 0 && coef_val != 1) {
                        // Extract LSB and add it to our byte
                        int bit = coef_val & 1;
                        current_byte |= (bit << bit_counter);
                        bit_counter++;
                        
                        if (bit_counter == 8) {
                            extracted_data.push_back(current_byte);
                            current_byte = 0;
                            bit_counter = 0;

                            //once we have enough bytes, parse the metadata to find the total size
                            if (!parsed && extracted_data.size() >= 5) { //1 for ext_len + 4 for file_size
                                ext_len = extracted_data[0];
                                if (extracted_data.size() >= 1 + ext_len + sizeof(uint32_t)) {
                                    memcpy(&file_size, &extracted_data[1 + ext_len], sizeof(uint32_t));
                                    total_size = 1 + ext_len + sizeof(uint32_t) + file_size;
                                    parsed = true;
                                }
                            }
                        }
                        //if we have extracted the entire package, stop
                        if (parsed && extracted_data.size() == total_size) {
                            goto end_extraction;
                        }
                    }
                }
            }
        }
    }
    end_extraction:; //jump to this label when conditions met

    //cleanup
    jpeg_finish_decompress(&decompress_info);
    jpeg_destroy_decompress(&decompress_info);
    fclose(encoded);

    if (!parsed or extracted_data.size() != total_size){
        std::cerr << "Error: Failed to extract complete package or file was not encoded using StegaSaur." << std::endl;
        return false;
    }

    //parse extracted data to write file
    size_t offset = 0;
    current_byte = 0;
    //get file extension
    ext_len = extracted_data[offset++];
    std::string file_ext(extracted_data.begin() + offset, extracted_data.begin() + offset + ext_len);
    offset += ext_len;
    if (file_ext == ".png" or file_ext == ".jpeg" or file_ext == ".jpg"){
        std::cout << "Console: Image detected. Extracting dimensions." << std::endl;
        unsigned char* height_bytes = reinterpret_cast<unsigned char*>(&height);
        for (int i = 0; i < sizeof(height); ++i){
            for (int j = 0; j < 8; ++j){
                unsigned char bit = file_data[offset] & 1;
                current_byte |= (bit << j);
                offset++;
            }
            height_bytes[i] = current_byte;
            current_byte = 0;
        }
        std::cout << "Console: Extracted height: " << height << std::endl;

        unsigned char* width_bytes = reinterpret_cast<unsigned char*>(&width);
        for (int i = 0; i < sizeof(width); ++i){
            for (int j = 0; j < 8; ++j){
                unsigned char bit = file_data[offset] & 1;
                current_byte |= (bit << j);
                offset++;
            }
            width_bytes[i] = current_byte;
            current_byte = 0;
        }
        std::cout << "Console: Extracted width: " << width << std::endl;
    }
    //get file size
    offset += sizeof(uint32_t);
    //get file data
    file_data.insert(file_data.begin(),extracted_data.begin() + offset, extracted_data.end());

    //write file
    if (file_ext == ".txt"){
        newFile = newFile+file_ext;
        encodedFile.setBinaryFileData(file_data);
        if(!encodedFile.writeFile(newFile)){
            return false;
        }
    }
    else if (file_ext == ".png"){
        encodedFile.setPngPixelData(file_data);
        encodedFile.setImageDimensions(0, height);
        encodedFile.setImageDimensions(1, width);
        newFile = newFile + file_ext;
        if (!encodedFile.writePng(newFile)){
            std::cerr << "Error: Failed to write to " << newFile << std::endl;
            return false;
        }
    }
    return true;
}