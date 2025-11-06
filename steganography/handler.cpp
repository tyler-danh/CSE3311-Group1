#include <iostream>
#include <string>
#include <filesystem>
#include <algorithm>
#include "handler.hpp"

//handlerexception implementation
HandlerException::HandlerException(ErrorCode code, const std::string& message) 
    : error_code(code), error_message(message) {}

bool Handler::validateCarrierContent(const std::string& filename) {
    //this read file header for magic number validation
    std::vector<char> header = readFileHeader(filename, 16); // read first 16 bytes
}
const char* HandlerException::what() const noexcept {
    return error_message.c_str();
}

ErrorCode HandlerException::getErrorCode() const noexcept {
    return error_code;
}

Handler::Handler(std::string file_name) : last_error(ErrorCode::SUCCESS) {
    this->file_name = file_name;
    
    //basic file existence check only - no automatic type validation
    if (!fileExists(file_name)) {
        throw HandlerException(ErrorCode::FILE_NOT_FOUND, 
                             "File not found: " + file_name);
    }
}
void Handler::parseExt(){
    //extract file extension using filesystem library
    std::string ext = std::filesystem::path(file_name).extension().string();
    file_ext = ext;
}
std::string Handler::getExt() const{
    return file_ext;
}
std::vector<char> Handler::readFile() {
    std::ifstream file(file_name, std::ios::binary | std::ios::ate);
    if(!file.is_open()){
        std::cerr << "ERR_FILE_OPEN_FAILED: Could not open file '" << file_name << "'" << std::endl;
        std::cerr << "Suggestion: Verify the file path is correct and you have read permissions" << std::endl;
        return false;
    }

    file_size = file.tellg();
    
    //check for empty file
    if(file_size == 0) {
        file.close();
        setError(ErrorCode::EMPTY_FILE, "File is empty: " + file_name);
        throw HandlerException(ErrorCode::EMPTY_FILE, "File is empty: " + file_name);
    }
    
    file.seekg(0, std::ios::beg);
    //this is a shortcut to get the filesize instead of incrementing a loop while reading
    //open and move pos to end, check pos for filesize, then move to beg

    binary_file_data.resize(file_size); //update buffer size based on file
    if(!file.read(reinterpret_cast<char*>(binary_file_data.data()), file_size)){
        std::cerr << "ERR_FILE_READ_FAILED: Could not read file '" << file_name << "' (size: " << file_size << " bytes)" << std::endl;
        std::cerr << "Suggestion: The file may be corrupted or locked by another process" << std::endl;
        return false;
    }
    
    file.close();
    return true;
}
bool Handler::readPng(){
    if (file_ext != ".png"){
        std::cerr << "ERR_INVALID_FILE_TYPE: File '" << file_name << "' is not a PNG file" << std::endl;
        std::cerr << "Suggestion: Ensure the file has a .png extension and is a valid PNG image" << std::endl;
        return false; 
    }
    FILE* image_file = fopen(file_name.c_str(), "rb"); //convert file_name to char
    if (!image_file){
        std::cerr << "ERR_FILE_OPEN_FAILED: Could not open PNG file '" << file_name << "'" << std::endl;
        std::cerr << "Suggestion: Verify the file exists and you have read permissions" << std::endl;
        return false;
    }
    //init png structs
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png){
        std::cerr << "ERR_LIBPNG_INIT_FAILED: libpng read struct failed to initialize for '" << file_name << "'" << std::endl;
        std::cerr << "Suggestion: This is an internal library error. Try reinstalling libpng" << std::endl;
        fclose(image_file);
        return false;
    }
    png_infop png_info = png_create_info_struct(png);
    if (!png_info){
        png_destroy_read_struct(&png, NULL, NULL);
        std::cerr << "ERR_LIBPNG_INIT_FAILED: libpng info struct failed to initialize for '" << file_name << "'" << std::endl;
        std::cerr << "Suggestion: This is an internal library error. Try reinstalling libpng" << std::endl;
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

    //read image data into image_pixel_data and close file
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
        std::cerr << "ERR_INVALID_FILE_TYPE: File '" << file_name << "' is not a JPEG/JPG file" << std::endl;
        std::cerr << "Suggestion: Ensure the file has a .jpeg or .jpg extension and is a valid JPEG image" << std::endl;
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
        std::cerr << "ERR_FILE_OPEN_FAILED: Could not open JPEG file '" << file_name << "'" << std::endl;
        std::cerr << "Suggestion: Verify the file exists and you have read permissions" << std::endl;
        jpeg_destroy_decompress(&decompress_info);
        return false;
    }
    jpeg_stdio_src(&decompress_info, image_file);

    //read jpeg header to get image info
    (void) jpeg_read_header(&decompress_info, TRUE);
    //decompress image
    (void) jpeg_start_decompress(&decompress_info);

    image_height = decompress_info.output_height;
    image_width = decompress_info.output_width;
    int c_channels = decompress_info.output_components; //3 for rgb

    image_pixel_data.resize(image_height * image_width * c_channels);
    file_size = image_pixel_data.size(); // Set file_size to pixel data size

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

//read whole file into binary_file_data and locate data chunk
bool Handler::readWav(){
    if (file_ext != ".wav"){
        std::cerr << "ERR_INVALID_FILE_TYPE: File '" << file_name << "' is not a WAV file" << std::endl;
        std::cerr << "Suggestion: Ensure the file has a .wav extension and is a valid WAV audio file" << std::endl;
        return false;
    }
    std::ifstream file(file_name, std::ios::binary | std::ios::ate);
    if(!file.is_open()){
        std::cerr << "ERR_FILE_OPEN_FAILED: Could not open WAV file '" << file_name << "'" << std::endl;
        std::cerr << "Suggestion: Verify the file exists and you have read permissions" << std::endl;
        return false;
    }
    file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    binary_file_data.resize(file_size);
    if(!file.read(reinterpret_cast<char*>(binary_file_data.data()), file_size)){
        std::cerr << "ERR_FILE_READ_FAILED: Could not read WAV file '" << file_name << "' (size: " << file_size << " bytes)" << std::endl;
        std::cerr << "Suggestion: The file may be corrupted or locked by another process" << std::endl;
        return false;
    }
    file.close();

    //find data chunk in WAV file
    wav_data_offset = 0;
    wav_data_size = 0;
    for (std::streamsize i = 0; i + 8 <= file_size; ++i){
        if (binary_file_data[i] == 'd' && binary_file_data[i+1] == 'a' && binary_file_data[i+2] == 't' && binary_file_data[i+3] == 'a'){
            std::uint32_t size = 0;
            size |= static_cast<std::uint32_t>(binary_file_data[i+4]);
            size |= static_cast<std::uint32_t>(binary_file_data[i+5]) << 8;
            size |= static_cast<std::uint32_t>(binary_file_data[i+6]) << 16;
            size |= static_cast<std::uint32_t>(binary_file_data[i+7]) << 24;
            wav_data_size = size;
            wav_data_offset = i + 8; // data starts after data + size field
            //some WAV files have a data chunk size of 0
            //in that case, use the remaining bytes in the file as the data chunk size
            if (wav_data_size == 0) {
                std::streamsize remaining = file_size - wav_data_offset;
                if (remaining > 0) wav_data_size = static_cast<std::uint32_t>(remaining);
            }
            break;
        }
    }
    if (wav_data_offset == 0){
        std::cerr << "ERR_CORRUPTED_DATA: Could not find data chunk in WAV file '" << file_name << "'" << std::endl;
        std::cerr << "Suggestion: The WAV file may be corrupted or in an unsupported format" << std::endl;
        return false;
    }
    return true;
}
    //----------WRITING----------
bool Handler::writeFile(const std::string name){
    std::ofstream file(name, std::ios::binary);
    if(!file.is_open()){
        std::cerr << "ERR_FILE_OPEN_FAILED: Could not open file '" << name << "' for writing" << std::endl;
        std::cerr << "Suggestion: Check that you have write permissions in the target directory" << std::endl;
        return false;
    }

    file.write(reinterpret_cast<const char*>(binary_file_data.data()), binary_file_data.size());

    if(!file.good()){
        std::cerr << "ERR_FILE_WRITE_FAILED: Failed to write to file '" << name << "' (" << binary_file_data.size() << " bytes)" << std::endl;
        std::cerr << "Suggestion: Ensure you have sufficient disk space and write permissions" << std::endl;
        return false;
    }
    file.close();
    return true;
}
    //write wav replace data chunk bytes with sample_data in binary_file_data and write whole file
bool Handler::writeWav(const std::string name){
    if (file_ext != ".wav" && name.find(".wav") == std::string::npos){
        std::cerr << "ERR_INVALID_FILE_TYPE: Cannot write '" << name << "' as WAV file (invalid extension)" << std::endl;
        std::cerr << "Suggestion: Ensure the output filename has a .wav extension" << std::endl;
        return false;
    }
    if (wav_data_offset == 0 || wav_data_size == 0){
        std::cerr << "ERR_CORRUPTED_DATA: WAV data chunk not initialized for '" << name << "'" << std::endl;
        std::cerr << "Suggestion: The WAV file structure may be corrupted. Try using a different carrier file" << std::endl;
        return false;
    }
    //write binary_file_data to file
    std::ofstream file(name, std::ios::binary);
    if (!file.is_open()){
        std::cerr << "ERR_FILE_OPEN_FAILED: Could not open WAV file '" << name << "' for writing" << std::endl;
        std::cerr << "Suggestion: Check that you have write permissions in the target directory" << std::endl;
        return false;
    }
    file.write(reinterpret_cast<const char*>(binary_file_data.data()), binary_file_data.size());
    file.close();
    return true;
}
bool Handler::writePng(const std::string name){
    //this function assumes image is in simple RGBA format
    //find again because of earlier issue with .contains()
    if (name.find(".png") == std::string::npos){
        std::cerr << "ERR_INVALID_FILE_TYPE: Cannot write '" << name << "' as PNG file (invalid extension)" << std::endl;
        std::cerr << "Suggestion: Ensure the output filename has a .png extension" << std::endl;
        return false;
    }
    FILE* image_file = fopen(name.c_str(), "wb");
    if(!image_file){
        std::cerr << "ERR_FILE_OPEN_FAILED: Could not open PNG file '" << name << "' for writing" << std::endl;
        std::cerr << "Suggestion: Check that you have write permissions in the target directory" << std::endl;
        return false;
    }

    //init libpng write structs
    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png){
        std::cerr << "ERR_LIBPNG_INIT_FAILED: libpng write struct failed to initialize for '" << name << "'" << std::endl;
        std::cerr << "Suggestion: This is an internal library error. Try reinstalling libpng" << std::endl;
        fclose(image_file);
        return false;
    }
    png_infop png_info = png_create_info_struct(png);
    if (!png_info){
        png_destroy_write_struct(&png, NULL);
        std::cerr << "ERR_LIBPNG_INIT_FAILED: libpng write info struct failed to initialize for '" << name << "'" << std::endl;
        std::cerr << "Suggestion: This is an internal library error. Try reinstalling libpng" << std::endl;
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
        std::cerr << "ERR_CORRUPTED_DATA: PNG data size does not match dimensions for '" << name << "'" << std::endl;
        std::cerr << "Expected size: " << (size_t)image_width * image_height * 4 << " bytes, Actual size: " << image_pixel_data.size() << " bytes" << std::endl;
        std::cerr << "Suggestion: The image data may be corrupted. Try using a different carrier file" << std::endl;
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
    //if(file_ext != ".jpeg" and file_ext != ".jpg"){
    //std::cerr << "ERR_INVALID_FILE_TYPE: File '" << file_name << "' is not a JPEG/JPG file" << std::endl;
    //return false;
    //}
    //init jpeg structs for compression
    struct jpeg_compress_struct compress_info;
    struct jpeg_error_mgr jpeg_err;

    compress_info.err = jpeg_std_error(&jpeg_err);
    jpeg_create_compress(&compress_info);

    //create output file
    FILE* image_file = fopen(name.c_str(), "wb");
    if(!image_file){
        std::cerr << "ERR_FILE_OPEN_FAILED: Could not open JPEG file '" << name << "' for writing" << std::endl;
        std::cerr << "Suggestion: Check that you have write permissions in the target directory" << std::endl;
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

void Handler::setWavSampleData(std::vector<unsigned char> sample_data){
    if (wav_data_offset == 0 || wav_data_size == 0) return;
    for (std::uint32_t i = 0; i < wav_data_size && i < sample_data.size(); ++i){
        binary_file_data[wav_data_offset + i] = sample_data[i];
    }
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
std::vector<unsigned char> Handler::getWavSampleData() const{
    if (wav_data_offset == 0 || wav_data_size == 0) return std::vector<unsigned char>();
    return std::vector<unsigned char>(binary_file_data.begin() + wav_data_offset, binary_file_data.begin() + wav_data_offset + wav_data_size);
}
std::vector<unsigned char> Handler::getFileData() const{
>>>>>>> Stashed changes
    return binary_file_data;
}

bool Handler::writeFile(const std::string name, const std::vector<char> binary){
    std::ofstream file(name, std::ios::binary);
    if(!file.is_open()){
        setError(ErrorCode::FILE_ACCESS_ERROR, "Could not open file for writing: " + name);
        throw HandlerException(ErrorCode::FILE_ACCESS_ERROR, "Could not open file for writing: " + name);
    }

    file.write(binary.data(), binary.size());

    if(!file.good()){
        file.close();
        setError(ErrorCode::WRITE_ERROR, "Failed to write to file: " + name);
        throw HandlerException(ErrorCode::WRITE_ERROR, "Failed to write to file: " + name);
    }
    
    file.close();
    clearError(); //clear any previous errors on successful write
    return true;
}

//file validation methods
bool Handler::validateSecretFile() {
    try {
        //for secret files, we only need basic validation
        if (!fileExists(file_name)) {
            setError(ErrorCode::FILE_NOT_FOUND, "Secret file not found: " + file_name);
            return false;
        }
        
        //check if file is empty
        std::ifstream file(file_name, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            setError(ErrorCode::FILE_ACCESS_ERROR, "Cannot access secret file: " + file_name);
            return false;
        }
        
        auto size = file.tellg();
        file.close();
        
        if (size == 0) {
            setError(ErrorCode::EMPTY_FILE, "Secret file is empty: " + file_name);
            return false;
        }
        
        clearError();
        return true;
        
    } catch (const std::exception& e) {
        setError(ErrorCode::FILE_ACCESS_ERROR, "Error validating secret file: " + std::string(e.what()));
        return false;
    }
}

bool Handler::validateCarrierFile() {
    try {
        //basic file existence and access check
        if (!fileExists(file_name)) {
            setError(ErrorCode::FILE_NOT_FOUND, "Carrier file not found: " + file_name);
            return false;
        }
        
        //validate extension
        if (!validateCarrierExtension(file_name)) {
            setError(ErrorCode::INVALID_CARRIER_TYPE, 
                    "Invalid carrier file type. Must be PNG, JPG/JPEG, WAV, or MP4: " + file_name);
            return false;
        }
        
        //validate file content
        if (!validateCarrierContent(file_name)) {
            setError(ErrorCode::INVALID_FILE_CONTENT, 
                    "Carrier file content does not match extension: " + file_name);
            return false;
        }
        
        clearError();
        return true;
        
    } catch (const std::exception& e) {
        setError(ErrorCode::FILE_ACCESS_ERROR, "Error validating carrier file: " + std::string(e.what()));
        return false;
    }
}

//error handling methods
ErrorCode Handler::getLastError() const {
    return last_error;
}

std::string Handler::getLastErrorMessage() const {
    return last_error_message;
}

bool Handler::hasError() const {
    return last_error != ErrorCode::SUCCESS;
}

void Handler::clearError() {
    last_error = ErrorCode::SUCCESS;
    last_error_message.clear();
}

//helper methods
void Handler::setError(ErrorCode code, const std::string& message) {
    last_error = code;
    last_error_message = message;
}

bool Handler::fileExists(const std::string& filename) const {
    return std::filesystem::exists(filename) && std::filesystem::is_regular_file(filename);
}

bool Handler::validateCarrierExtension(const std::string& filename) {
    std::string ext = std::filesystem::path(filename).extension().string();
    //convert to lowercase for case-insensitive comparison
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    return (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".wav" || ext == ".mp4");
}

bool Handler::validateCarrierContent(const std::string& filename) {
    //read file header for magic number validation
    std::vector<char> header = readFileHeader(filename, 16); // Read first 16 bytes
    
    if (header.empty()) {
        return false;
    }
    
    std::string ext = std::filesystem::path(filename).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    //validate content matches extension
    if (ext == ".png" && isPNG(header)) return true;
    if ((ext == ".jpg" || ext == ".jpeg") && isJPEG(header)) return true;
    if (ext == ".wav" && isWAV(header)) return true;
    if (ext == ".mp4" && isMP4(header)) return true;
    
    return false;
}

std::vector<char> Handler::readFileHeader(const std::string& filename, size_t bytes) {
    std::vector<char> header;
    std::ifstream file(filename, std::ios::binary);
    
    if (!file.is_open()) {
        return header;
    }
    
    header.resize(bytes);
    file.read(header.data(), bytes);
    header.resize(file.gcount()); //Resize to actual bytes read
    
    return header;
}

bool Handler::isPNG(const std::vector<char>& data) {
    //PNG magic number: 89 50 4E 47 0D 0A 1A 0A
    const std::vector<unsigned char> png_signature = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    
    if (data.size() < png_signature.size()) return false;
    
    for (size_t i = 0; i < png_signature.size(); ++i) {
        if (static_cast<unsigned char>(data[i]) != png_signature[i]) {
            return false;
        }
    }
    return true;
}

bool Handler::isJPEG(const std::vector<char>& data) {
    //JPEG magic number: FF D8 FF
    if (data.size() < 3) return false;
    
    return (static_cast<unsigned char>(data[0]) == 0xFF && 
            static_cast<unsigned char>(data[1]) == 0xD8 && 
            static_cast<unsigned char>(data[2]) == 0xFF);
}

bool Handler::isWAV(const std::vector<char>& data) {
    //WAV magic number: "RIFF" at start and "WAVE" at offset 8
    if (data.size() < 12) return false;
    
    return (data[0] == 'R' && data[1] == 'I' && data[2] == 'F' && data[3] == 'F' &&
            data[8] == 'W' && data[9] == 'A' && data[10] == 'V' && data[11] == 'E');
}

bool Handler::isMP4(const std::vector<char>& data) {
    //MP4 magic number: Check for "ftyp" at offset 4
    if (data.size() < 8) return false;
    
    return (data[4] == 'f' && data[5] == 't' && data[6] == 'y' && data[7] == 'p');
}
