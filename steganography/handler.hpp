#ifndef ENCODE_H
#define ENCODE_H

#include <string>
#include <vector>
#include <fstream>

class Handler{
    public:
        Handler(const std::string file_name);
        void parseExt();
        std::string getExt() const;
        std::vector<char> readFile();
        bool writeFile(const std::string name, const std::vector<char> binary);
        
    private:
        std::string file_name, file_ext;
        std::vector<char> binary_file_data;
        std::streamsize file_size;
};

#endif