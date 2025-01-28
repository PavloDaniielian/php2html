#pragma once

#ifndef CONVERT_H
#define CONVERT_H

#include <string>
#include <windows.h>

// Function to start the conversion process
void StartConversion(HWND hwnd, const std::string& phpDir, const std::string& templateDir, const std::string& htmlDir, const std::string& productName,
    const std::string& classesToKeep, const std::string& replaceDir, bool deleteUncompressedFiles, const std::string& yourLink);

#endif // CONVERT_H
