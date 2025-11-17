#include <iostream>
#include <vector>
#include "decoder.hpp"
#include <cstdint>
#include <cstring>
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
    else if (encodedFile.getExt() == ".wav"){
        file_check = encodedFile.readWav();
        file_data = encodedFile.getWavSampleData();
    }
    else if (encodedFile.getExt() == ".jpeg" or encodedFile.getExt() == ".jpg"){
        //must do actual checking in decoding method
        file_check = true;
    }
    if (file_check == false){
        std::cerr << "Error: Encoded file failed to open" << std::endl;
        return false;
    }
    return true;
}
bool Decoder::checksumCheck(uint16_t checksum){
    if (checksum % 13 == 0){return true;}
    return false;
}
bool Decoder::pngDecode(std::string newFile){
    std::streamsize offset = 0;
    uint8_t ext_len = 0;
    uint16_t checksum = 0;
    unsigned char extracted_byte = 0;
    int height = 0, width = 0;

    //get checksum and check it
    unsigned char* checksum_bytes = reinterpret_cast<unsigned char*>(&checksum);
    for (size_t i = 0; i < sizeof(checksum); ++i){
        for (int j = 0; j < 8; ++j){
            unsigned char bit = file_data[offset] & 1;
            extracted_byte |= (bit << j);
            offset++;
        }
        checksum_bytes[i] = extracted_byte;
        extracted_byte = 0;
    }
    std::cout << checksum;
    checksum = extracted_byte;
    if(!checksumCheck(checksum)){
        std::cerr << "Error: Checksum has been tampered with or invalid." << "\n The file does not contained encoded data, has not been encoded with StegaSaur, or encoded data has been tampered with." << std::endl;
        return false;
    }
    else{
        std::cout << "Console: Checksum verified. Continuing extraction." << std::endl;
    }
    extracted_byte = 0;
    //next, get ext_len
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
        //changed int to size_t for sizeof compatibility
            for (size_t i = 0; i < sizeof(height); ++i){
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
        //changed int to size_t for sizeof compatibility
        for (size_t i = 0; i < sizeof(width); ++i){
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
    //changed int to size_t for sizeof compatibility
    for (size_t i = 0; i < sizeof(data_size); ++i){
        for (size_t j = 0; j < 8; ++j){
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
    //changed int to uint32_t for data_size compatibility
    for (uint32_t i = 0; i < data_size; ++i){
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
    else if (file_ext == ".wav"){
        encodedFile.setBinaryFileData(extracted_data);
        newFile = newFile + file_ext;
        if(!encodedFile.writeFile(newFile)){
            std::cerr << "Error: Failed to write to " << newFile << std::endl;
            return false;
        }
    }
    else if (file_ext == ".jpeg" or file_ext == ".jpg"){
        encodedFile.setPngPixelData(extracted_data);
        encodedFile.setImageDimensions(0, height);
        encodedFile.setImageDimensions(1, width);
        newFile = newFile + file_ext;
        if (!encodedFile.writeJpeg(newFile)){
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
    uint16_t checksum = 0;
    uint8_t ext_len = 0;
    uint32_t file_size = 0;
    size_t total_size = 0;
    int height = 0, width = 0;
    bool parsed = false; //once we have enough data mark as true to stop iterating

//JSTEG extraction logic
    for (int comp_i = 0; comp_i < decompress_info.num_components; ++comp_i){
        for (JDIMENSION block_y = 0; block_y < decompress_info.comp_info[comp_i].height_in_blocks; ++block_y) {
            JBLOCKARRAY block_array = (decompress_info.mem->access_virt_barray)((j_common_ptr)&decompress_info, coeffcients[comp_i], block_y, 1, FALSE);
            for (JDIMENSION block_x = 0; block_x < decompress_info.comp_info[comp_i].width_in_blocks; ++block_x) {
                for (int i = 0; i < DCTSIZE2; ++i) {
                    JCOEF coef_val = block_array[0][block_x][i];

                    //JSTEG Rule: Only extract from coefficients that are not 0 or 1
                    if (coef_val != 0 && coef_val != 1) {
                        // Extract LSB and add it to our byte
                        int bit = coef_val & 1;
                        current_byte |= (bit << bit_counter);
                        bit_counter++;
                        
                        if (bit_counter == 8){
                            extracted_data.push_back(current_byte);
                            current_byte = 0;
                            bit_counter = 0;
                            if (!parsed) {
                                //define sizes, these are minimum sizes that will be extracted
                                //extracting currently has no verification, will be done later
                                const size_t CHECKSUM_SIZE = sizeof(uint16_t); // 2
                                const size_t EXT_LEN_SIZE = sizeof(uint8_t);  // 1
                                const size_t FILE_SIZE_SIZE = sizeof(uint32_t); // 4
                                const size_t IMG_DIMS_SIZE = sizeof(int) * 2; // 8

                                //check if we have the minimal header (checksum + ext_len)
                                if (extracted_data.size() >= CHECKSUM_SIZE + EXT_LEN_SIZE){
                                    ext_len = extracted_data[CHECKSUM_SIZE];
                                    size_t base_header_size = CHECKSUM_SIZE + EXT_LEN_SIZE + ext_len;

                                    //check if we have the base header (checksum + ext_len + ext)
                                    if (extracted_data.size() >= base_header_size){
                                        //we must read the extension now to check it
                                        std::string temp_ext(extracted_data.begin() + CHECKSUM_SIZE + EXT_LEN_SIZE,
                                                             extracted_data.begin() + base_header_size);
                                        
                                        size_t total_header_size = base_header_size;
                                        if (temp_ext == ".png" || temp_ext == ".jpeg" || temp_ext == ".jpg"){
                                            total_header_size += IMG_DIMS_SIZE;
                                        }

                                        //check if we have the full header (all metadata)
                                        if (extracted_data.size() >= total_header_size + FILE_SIZE_SIZE){
                                            //read the file size
                                            memcpy(&file_size, &extracted_data[total_header_size], FILE_SIZE_SIZE);
                                            
                                            //calculate total size and set the flag
                                            total_size = total_header_size + FILE_SIZE_SIZE + file_size;
                                            parsed = true;
                                        }
                                    }
                                }
                            }
                        }

                        //if we have extracted the entire package, stop
                        if (parsed && extracted_data.size() == total_size){
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
    //get checksum
    memcpy(&checksum, &extracted_data[offset], sizeof(uint16_t));
    //check checksum
    if(!checksumCheck(checksum)){
        std::cerr << "Error: Checksum has been tampered with or invalid." << "\n The file does not contained encoded data, has not been encoded with StegaSaur, or encoded data has been tampered with." << std::endl;
        return false;
    }
    else{
        std::cout << "Console: Checksum verified. Continuing extraction." << std::endl;
    }
    offset += sizeof(checksum);
    //get file extension
    ext_len = extracted_data[offset];
    offset += sizeof(ext_len);
    std::string file_ext = "";
    file_ext.assign(extracted_data.begin() + offset, extracted_data.begin() + offset + ext_len);
    offset += ext_len;
    if (file_ext == ".png" or file_ext == ".jpeg" or file_ext == ".jpg"){
        std::cout << "Console: Image detected. Extracting dimensions." << std::endl;
        memcpy(&height, &extracted_data[offset], sizeof(int));
        offset += sizeof(height);
        std::cout << "Console: Extracted height: " << height << std::endl;
        memcpy(&width, &extracted_data[offset], sizeof(int));
        offset += sizeof(width);
        std::cout << "Console: Extracted width: " << width << std::endl;
    }
    //get file size
    memcpy(&file_size, &extracted_data[offset], sizeof(uint32_t));
    offset += sizeof(file_size);
    //get file data
    file_data.insert(file_data.begin(), extracted_data.begin() + offset, extracted_data.end());

    //write file
    if (file_ext == ".txt"){
        newFile = newFile+file_ext;
        encodedFile.setBinaryFileData(file_data);
        if(!encodedFile.writeFile(newFile)){
            return false;
        }
        std::cout << "Console: Successfully extracted to " << newFile << std::endl;
        return true;
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
        std::cout << "Console: Successfully extracted to " << newFile << std::endl;
        return true;
    }
    return false;
}