#include <iostream>
#include <string>
#include "handler.hpp"

Handler::Handler(std::string file_name){
    this->file_name = file_name;
}
void Handler::parseExt(){
    //check file extentions of the secret file
    if(file_name.contains(".txt")){file_ext=".txt";}
    else if(file_name.contains(".png")){file_ext=".png";}
    else if(file_name.contains(".jpeg")){file_ext=".jpeg";}
    else if(file_name.contains(".jpg")){file_ext=".jpg";}
    else{file_ext = "INVALID";}
}
std::string Handler::getExt() const{
    return file_ext;
}
std::vector<char> Handler::readFile(){
    std::ifstream file(file_name, std::ios::binary | std::ios::ate);
    if(!file.is_open()){
        std::cerr << "Error: Could not open " << file_name << std::endl;
        return binary_file_data;
    }

    file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    //this is a shortcut to get the filesize instead of incrementing a loop while reading
    //open and move pos to end, check pos for filesize, then move to beg

    binary_file_data.resize(file_size); //update buffer size based on file
    if(!file.read(binary_file_data.data(), file_size)){
        std::cerr << "Error: Could not read " << file_name << std::endl;
        return binary_file_data;
    }
    file.close();
    return binary_file_data;
}

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
