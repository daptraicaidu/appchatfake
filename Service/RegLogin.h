// RegLogin.h

#pragma once
#include <string>

// Các mã trả về cho client
enum class AuthResponse {
    SUCCESS_LOGIN,
    SUCCESS_REGISTER,
    FAILED_USER_EXISTS,     
    FAILED_NOT_FOUND,      
    FAILED_WRONG_PASS, 
    FAILED_DB_ERROR,       
    FAILED_INVALID_INPUT    
};


AuthResponse ProcessRegistration(const std::string& username, const std::string& password);

AuthResponse ProcessLogin(const std::string& username, const std::string& password);