#include <iostream>
#include <string>
#include "handler.hpp"
#include "encoder.hpp"
#include "decoder.hpp"
#include <ctime>
#include <algorithm>

int main(){
    std::string secret, carrier;
    time_t timestamp = time(0);
    std::cout << "Enter Secret file: " << std::endl;
    std::cin >> secret;
    std::cout << "Enter Carrier file: " << std::endl;
    std::cin >> carrier;
    std::string current_time = ctime(&timestamp);

    Encoder stega = Encoder(secret, carrier);
    if (stega.openFiles()){
        std::cout << "Console: Files successfully opened" << std::endl;
        current_time.erase(std::remove(current_time.begin(), current_time.end(), ' '), current_time.end());
        current_time.erase(std::remove(current_time.begin(), current_time.end(), ':'), current_time.end());
        current_time.erase(std::remove(current_time.begin(), current_time.end(), '\n'), current_time.end());
        //current_time = current_time + ".png";
        //commented out above line to prevent issues with .wav carrier
        //use carrier's extension for output (png or wav etc.)
    std::string carrier_ext = ".png";
    size_t pos = carrier.rfind('.');
    if (pos != std::string::npos) carrier_ext = carrier.substr(pos);
    current_time = current_time + carrier_ext;

        // choose encoder based on carrier extension
        if (carrier_ext == ".png"){
            if (stega.pngLsb(current_time)){
                std::cout << "File " << current_time << " created succesfully" << std::endl;
            }
        } else if (carrier_ext == ".jpeg" || carrier_ext == ".jpg"){
            if (stega.dctJpeg(current_time)){
                std::cout << "File " << current_time << " created succesfully" << std::endl;
            }
        } else {
            // fallback to png LSB (may fail for unsupported carriers)
            if (stega.pngLsb(current_time)){
                std::cout << "File " << current_time << " created succesfully" << std::endl;
            }
        }
    }
    
    Decoder saur = Decoder(current_time);
    if(saur.openEncodedFile()){
        // choose decode path based on encoded file extension
        size_t pos = current_time.rfind('.');
        std::string ext = ".png";
        if (pos != std::string::npos) ext = current_time.substr(pos);
        bool ok = false;
        if (ext == ".png"){
            ok = saur.pngDecode("decode");
        } else if (ext == ".jpeg" || ext == ".jpg"){
            ok = saur.jpegDecode("decode");
        } else {
            // fallback to png decoder
            ok = saur.pngDecode("decode");
        }
        if (ok){
            std::cout << "decoded!" << std::endl;
        }
    }
    return 0;
}