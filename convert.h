#pragma once

#ifndef CONVERT_H
#define CONVERT_H

#include <string>
#include <windows.h>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <regex>
#include <commctrl.h>
#include <map>
namespace fs = std::filesystem;


#define MAX_EMAILS_NUM  7
#define EMAILS_FOOT_Y   (290 + 30 * MAX_EMAILS_NUM + 10)

#define STR_PHP_CONTENT_start "start of reseller site"
#define STR_PHP_CONTENT_end "end of reseller site"

// Function to start the conversion process
void StartConversion(HWND hwnd, const std::string& phpDir, const std::string& templateDir, const std::string& htmlDir, const std::string& productName,
    const std::string& classesToKeep, const std::string& replaceDir, std::map<std::string, std::string>  emailMap, const std::string& yourLink, bool deleteUncompressedFiles);

#endif // CONVERT_H
