#include <iostream>
#include <string>
#include "handler.hpp"
#include "encoder.hpp"
#include "decoder.hpp"
#include <ctime>
#include <algorithm>
#include <filesystem>

// Forward declarations for mode functions
bool encodeNonInteractive(const std::string& carrier, const std::string& secret, const std::string& output_base);
bool decodeNonInteractive(const std::string& encoded, const std::string& output_base);
void runInteractiveMode();

int main(int argc, char* argv[]){
    // Non-interactive mode for Streamlit integration
    if (argc == 5 && std::string(argv[1]) == "encode") {
        // Encode mode: ./stegasaur encode <carrier> <secret> <output_base>
        std::string carrier = argv[2];
        std::string secret = argv[3];
        std::string output_base = argv[4];
        
        if (encodeNonInteractive(carrier, secret, output_base)) {
            return 0;
        } else {
            return 1;
        }
    }
    else if (argc == 4 && std::string(argv[1]) == "decode") {
        // Decode mode: ./stegasaur decode <encoded> <output_base>
        std::string encoded = argv[2];
        std::string output_base = argv[3];
        
        if (decodeNonInteractive(encoded, output_base)) {
            return 0;
        } else {
            return 1;
        }
    }
    else if (argc == 1) {
        // Interactive mode
        runInteractiveMode();
        return 0;
    }
    else {
        // Invalid arguments
        std::cerr << "ERR_INVALID_ARGUMENTS: Invalid command-line arguments provided" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Usage:" << std::endl;
        std::cerr << "  Encode: " << argv[0] << " encode <carrier_file> <secret_file> <output_base>" << std::endl;
        std::cerr << "  Decode: " << argv[0] << " decode <encoded_file> <output_base>" << std::endl;
        std::cerr << "  Interactive: " << argv[0] << std::endl;
        std::cerr << std::endl;
        std::cerr << "Suggestion: Check that you provided the correct number of arguments and that 'encode' or 'decode' is specified" << std::endl;
        return 1;
    }
}

bool encodeNonInteractive(const std::string& carrier, const std::string& secret, const std::string& output_base) {
    Encoder stega = Encoder(secret, carrier);
    
    if (!stega.openFiles()) {
        std::cerr << "ERR_FILE_OPEN_FAILED: Could not open input files" << std::endl;
        std::cerr << "Suggestion: Verify that both files exist and you have read permissions" << std::endl;
        return false;
    }
    
    // Determine carrier type and encode accordingly (case-insensitive)
    std::string carrier_lower = carrier;
    for (char &c : carrier_lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    
    std::string output_file;
    bool success = false;
    
    if (carrier_lower.find(".png") != std::string::npos) {
        output_file = output_base + ".png";
        success = stega.pngLsb(output_file);
    }
    else if (carrier_lower.find(".wav") != std::string::npos) {
        output_file = output_base + ".wav";
        success = stega.pngLsb(output_file);  // WAV uses same LSB method
    }
    else if (carrier_lower.find(".jpeg") != std::string::npos || carrier_lower.find(".jpg") != std::string::npos) {
        // dctJpeg appends .jpeg internally
        success = stega.dctJpeg(output_base);
        
        // Determine actual output filename
        std::string created = output_base + ".jpeg";
        output_file = created;
        
        // If carrier was .jpg, rename to .jpg for consistency
        if (carrier.find(".jpg") != std::string::npos) {
            std::string preferred = output_base + ".jpg";
            if (std::rename(created.c_str(), preferred.c_str()) == 0) {
                output_file = preferred;
            }
        }
    }
    else {
        std::cerr << "ERR_INVALID_FILE_TYPE: Unsupported carrier file type '" << carrier << "'" << std::endl;
        std::cerr << "Suggestion: Use PNG, JPEG, JPG, or WAV files as carriers" << std::endl;
        return false;
    }
    
    if (success) {
        // Print only the output filename to stdout
        std::cout << output_file << std::endl;
        return true;
    } else {
        std::cerr << "ERR_ENCODING_FAILED: Encoding operation failed for carrier '" << carrier << "'" << std::endl;
        std::cerr << "Suggestion: Check that the carrier file is valid and not corrupted" << std::endl;
        return false;
    }
}

bool decodeNonInteractive(const std::string& encoded, const std::string& output_base) {
    Decoder saur = Decoder(encoded);
    
    if (!saur.openEncodedFile()) {
        std::cerr << "ERR_FILE_OPEN_FAILED: Could not open encoded file '" << encoded << "'" << std::endl;
        std::cerr << "Suggestion: Verify that the file exists and you have read permissions" << std::endl;
        return false;
    }
    
    // Determine file type and decode accordingly (case-insensitive)
    std::string encoded_lower = encoded;
    for (char &c : encoded_lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    
    bool success = false;
    
    if (encoded_lower.find(".png") != std::string::npos || encoded_lower.find(".wav") != std::string::npos) {
        success = saur.pngDecode(output_base);
    }
    else if (encoded_lower.find(".jpeg") != std::string::npos || encoded_lower.find(".jpg") != std::string::npos) {
        success = saur.jpegDecode(output_base);
    }
    else {
        std::cerr << "ERR_INVALID_FILE_TYPE: Unsupported encoded file type '" << encoded << "'" << std::endl;
        std::cerr << "Suggestion: Only PNG, JPEG, JPG, and WAV files can be decoded" << std::endl;
        return false;
    }
    
    if (success) {
        // The decoder methods append the extension internally based on extracted metadata
        // We output just the base name - the frontend will need to check what file was created
        // Common extensions are .txt, .png, .jpeg, .jpg
        std::cout << output_base << std::endl;
        return true;
    } else {
        std::cerr << "ERR_DECODING_FAILED: Decoding operation failed for file '" << encoded << "'" << std::endl;
        std::cerr << "Suggestion: Ensure the file was encoded with StegaSaur and has not been modified" << std::endl;
        return false;
    }
}

void runInteractiveMode() {
    std::string secret, carrier, new_file, encoded_file, mode;
    std::cout << "Welcome to the StegaSaur Steganography Command Line Interface!" << std::endl;
    
    while(1){
        std::cout << "Select mode:" << std::endl << "\t [1] Encoding" << std::endl << "\t [2] Decoding" << std::endl << "\t [3] Exit" << std::endl;
        std::cin >> mode;
        
        if (mode != "1" && mode != "2" && mode != "3"){
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
            
            // Build output path inside the same directory as the carrier (if carrier had a path)
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
            else if (carrier.find(".jpeg") != std::string::npos || carrier.find(".jpg") != std::string::npos){
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
                    if (std::rename(created.c_str(), preferred.c_str()) == 0){
                        final_out = preferred;
                    } else {
                        // rename failed, keep created name but notify
                        std::cerr << "Warning: failed to rename " << created << " to " << preferred << "." << std::endl;
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
            else if (encoded_file.find(".jpeg") != std::string::npos || encoded_file.find(".jpg") != std::string::npos){
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
}