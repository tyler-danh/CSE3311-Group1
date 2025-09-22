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
    
    // basic file existence check only - no automatic type validation
    if (!fileExists(file_name)) {
        throw HandlerException(ErrorCode::FILE_NOT_FOUND, 
                             "File not found: " + file_name);
    }
}
void Handler::parseExt(){
    // extract file extension using filesystem library
    std::string ext = std::filesystem::path(file_name).extension().string();
    file_ext = ext;
}
std::string Handler::getExt() const{
    return file_ext;
}
std::vector<char> Handler::readFile() {
    std::ifstream file(file_name, std::ios::binary | std::ios::ate);
    if(!file.is_open()){
        setError(ErrorCode::FILE_ACCESS_ERROR, "Could not open file: " + file_name);
        throw HandlerException(ErrorCode::FILE_ACCESS_ERROR, "Could not open file: " + file_name);
    }

    file_size = file.tellg();
    
    // check for empty file
    if(file_size == 0) {
        file.close();
        setError(ErrorCode::EMPTY_FILE, "File is empty: " + file_name);
        throw HandlerException(ErrorCode::EMPTY_FILE, "File is empty: " + file_name);
    }
    
    file.seekg(0, std::ios::beg);
    //this is a shortcut to get the filesize instead of incrementing a loop while reading
    //open and move pos to end, check pos for filesize, then move to beg

    binary_file_data.resize(file_size); //update buffer size based on file
    if(!file.read(binary_file_data.data(), file_size)){
        file.close();
        setError(ErrorCode::READ_ERROR, "Could not read file: " + file_name);
        throw HandlerException(ErrorCode::READ_ERROR, "Could not read file: " + file_name);
    }
    
    // check if we read the expected amount
    if(file.gcount() != file_size) {
        file.close();
        setError(ErrorCode::CORRUPTED_FILE, "File appears corrupted - read size mismatch: " + file_name);
        throw HandlerException(ErrorCode::CORRUPTED_FILE, "File appears corrupted - read size mismatch: " + file_name);
    }
    
    file.close();
    clearError(); //clear any previous errors on successful read
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
