#include <iostream>
#include <string>
#include "handler.hpp"
#include "encoder.hpp"
#include "decoder.hpp"
#include <ctime>
#include <algorithm>
#include <filesystem>

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
            // build output path inside the same directory as the carrier (if carrier had a path)
            std::string out_base = new_file;
            size_t sep_pos = carrier.find_last_of("\\/");
            if (sep_pos != std::string::npos){
                out_base = carrier.substr(0, sep_pos + 1) + new_file;
            }

            if (carrier.find(".png") != std::string::npos){
                std::string out_path = out_base + ".png";
                std::cout << "Console: PNG Carrier detected. Beginning PNG LSB method." << std::endl;
                if (!stega.pngLsb(out_path)){
                    std::cout << "Console: Aborting encoder." << std::endl;
                    continue;
                }
                std::cout << "Console: " << carrier << " successfully encoded and written to " << out_path << std::endl;
            }
            else if (carrier.find(".wav") != std::string::npos){
                std::string out_path = out_base + ".wav";
                std::cout << "Console: WAV Carrier detected. Beginning WAV LSB method." << std::endl;
                if (!stega.pngLsb(out_path)){
                    std::cout << "Console: Aborting encoder." << std::endl;
                    continue;
                }
                std::cout << "Console: " << carrier << " successfully encoded and written to " << out_path << std::endl;
            }
            else if (carrier.find(".jpeg") != std::string::npos or carrier.find(".jpg") != std::string::npos){
                std::cout << "Console: JPEG Carrier detected. Beginning JPEG DCT method." << std::endl;
                // dctJpeg will append .jpeg internally; pass base path (no ext)
                if (!stega.dctJpeg(out_base)){
                    std::cout << "Console: Aborting encoder." << std::endl;
                    continue;
                }
                // actual file created will be out_base + ".jpeg"
                std::string created = out_base + ".jpeg";
                std::string final_out = created;
                // if carrier was .jpg prefer .jpg for Windows users
                if (carrier.find(".jpg") != std::string::npos){
                    std::string preferred = out_base + ".jpg";
                    int r = std::rename(created.c_str(), preferred.c_str());
                    if (r == 0){
                        final_out = preferred;
                    } else {
                        // if rename failed, check whether the preferred file already exists (maybe created by OS or another process).
                        if (std::filesystem::exists(preferred)){
                            // preferred exists, consider it a success
                            final_out = preferred;
                        } else if (!std::filesystem::exists(created)){
                            // neither created nor preferred exists, something went wrong
                            std::cerr << "Warning: failed to rename " << created << " to " << preferred << "." << std::endl;
                        } else {
                            // created exists but rename failed for some reason; keep created and warn
                            std::cerr << "Warning: failed to rename " << created << " to " << preferred << "." << std::endl;
                        }
                    }
                }
                std::cout << "Console: " << carrier << " successfully encoded and written to " << final_out << std::endl;
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
            if (encoded_file.find(".png") != std::string::npos || encoded_file.find(".wav") != std::string::npos){
                if (!saur.pngDecode(new_file)){
                    std::cout << "Console: Aborting decoder." << std::endl;
                    continue;
                }
            }
            else if (encoded_file.find(".jpeg") != std::string::npos or encoded_file.find(".jpg") != std::string::npos){
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