#ifndef ENCODE_H
#define ENCODE_H

#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <filesystem>

enum class FileType {
    SECRET,
    CARRIER
};

enum class ErrorCode {
    SUCCESS = 0,
    INVALID_EXTENSION,
    FILE_NOT_FOUND,
    FILE_ACCESS_ERROR,
    EMPTY_FILE,
    CORRUPTED_FILE,
    WRITE_ERROR,
    READ_ERROR,
    INVALID_CARRIER_TYPE,
    INVALID_FILE_CONTENT,
    UNSUPPORTED_SECRET_TYPE
};

class HandlerException : public std::exception {
public:
    HandlerException(ErrorCode code, const std::string& message);
    const char* what() const noexcept override;
    ErrorCode getErrorCode() const noexcept;
    
private:
    ErrorCode error_code;
    std::string error_message;
};

class Handler{
    public:
        Handler(const std::string file_name);
        void parseExt();
        std::string getExt() const;
        std::vector<char> readFile();
        bool writeFile(const std::string name, const std::vector<char> binary);
        
        //file validation methods
        bool validateSecretFile();
        bool validateCarrierFile();
        
        //error handling methods
        ErrorCode getLastError() const;
        std::string getLastErrorMessage() const;
        bool hasError() const;
        void clearError();
        
    private:
        std::string file_name, file_ext;
        std::vector<char> binary_file_data;
        std::streamsize file_size;
        
        //error state tracking
        ErrorCode last_error;
        std::string last_error_message;
        
        //helper methods
        void setError(ErrorCode code, const std::string& message);
        bool fileExists(const std::string& filename) const;
        bool validateCarrierExtension(const std::string& filename);
        bool validateCarrierContent(const std::string& filename);
        bool isPNG(const std::vector<char>& data);
        bool isJPEG(const std::vector<char>& data);
        bool isWAV(const std::vector<char>& data);
        bool isMP4(const std::vector<char>& data);
        std::vector<char> readFileHeader(const std::string& filename, size_t bytes);
};

#endif