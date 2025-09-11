#include <iostream>
#include <string>
#include "encode.hpp"

std::string payload; //file to hide
std::string carrier; //file to hide in
int carrier_height, carrier_width = 0;

Encode::Encode(std::string secret, std::string carrier){
    this->secret_name = secret;
    this->carrier_name = carrier;
}
void Encode::parseExt(){
    //check file extentions of the secret file
    if(secret_name.contains(".txt")){secret_ext=".txt";}
    else if(secret_name.contains(".png")){secret_ext=".png";}
    else if(secret_name.contains(".jpeg") or secret_name.contains(".jpg")){secret_ext=".jpeg";}
    else{secret_ext = "INVALID";}
    std::cout << secret_ext;

    //check file extentions of the carrier file
    if (carrier_name.contains(".txt")){carrier_ext=".txt";}
    else if(carrier_name.contains(".png")){carrier_ext=".png";}
    else if(carrier_name.contains(".jpeg") or carrier_name.contains(".jpg")){carrier_ext=".jpeg";}
    else{carrier_ext = "INVALID";}
    std::cout << carrier_ext;
}
std::string Encode::getExt(std::string item) const{
    if (item == "secret"){return secret_ext;}
    else if(item == "carrier"){return carrier_ext;}
}