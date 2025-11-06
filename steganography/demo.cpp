#include <iostream>
#include <string>
#include "handler.hpp"
#include "encoder.hpp"
#include "decoder.hpp"
#include <ctime>
#include <algorithm>

int main(){
    std::string secret, carrier, new_file, encoded_file, mode;
    std::cout << "Welcome to the StegaSaur Steganography Command Line Interface!" << std::endl;
    while(1){
        std::cout << "Select mode:" << std::endl << "\t [1] Encoding" << std::endl << "\t [2] Decoding" << std::endl << "\t [3] Exit" << std::endl;
        std::cin >> mode;
        if (mode != "1" and mode != "2" and mode != "3"){
            std::cerr << "Error: Invalid mode. Select 1, 2, or 3." << std::endl;
            mode = "0";
            continue;
        }
        else if (mode == "1"){
            std::cout << "Console: Enter Secret file and Carrier files" << std::endl;
            std::cout << "Secret file: ";
            std::cin >> secret;
            std::cout << "Carrier file: ";
            std::cin >> carrier;
            std::cout << "Name your encoded file (Only file name, do not include extension): ";
            std::cin >> new_file;

            Encoder stega = Encoder(secret, carrier);
            if (!stega.openFiles()){
                std::cout << "Console: Aborting encoder." << std::endl;
                continue;
            }
            if (carrier.contains(".png")){
                std::cout << "Console: PNG Carrier detected. Beginning PNG LSB method." << std::endl;
                if (!stega.pngLsb(new_file)){
                    std::cout << "Console: Aborting encoder." << std::endl;
                    continue;
                }
                new_file = new_file + ".png";
                std::cout << "Console: " << carrier << " successfully encoded and written to " << new_file << std::endl;
            }
            else if (carrier.contains(".jpeg") or carrier.contains(".jpg")){
                std::cout << "Console: JPEG Carrier detected. Beginning JPEG DCT method." << std::endl;
                if (!stega.dctJpeg(new_file)){
                    std::cout << "Console: Aborting encoder." << std::endl;
                    continue;
                }
                new_file = new_file + ".jpeg";
                std::cout << "Console: " << carrier << " successfully encoded and written to " << new_file << std::endl;
            }
        }
        else if (mode == "2"){
            std::cout << "Console: Enter Encoded file" << std::endl;
            std::cout << "Encoded file: ";
            std::cin >> encoded_file;
            std::cout << "Name your decoded file (Only file name, do not include extension): ";
            std::cin >> new_file;

            Decoder saur = Decoder(encoded_file);
            if(!saur.openEncodedFile()){
                std::cout << "Console: Aborting decoder." << std::endl;
                continue;
            }
            if (encoded_file.contains(".png")){
                if (!saur.pngDecode(new_file)){
                    std::cout << "Console: Aborting decoder." << std::endl;
                    continue;
                }
            }
            else if (encoded_file.contains(".jpeg")){
                if (!saur.jpegDecode(new_file)){
                    std::cout << "Console: Aborting decoder." << std::endl;
                    continue;
                }
            }
        }
        else if (mode == "3"){
            std::cout << "Console: Exiting StegaSaur. Good bye!" << std::endl;
            exit(0);
        }

    }
    return 0;
}