#include "convert.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <regex>
#include <commctrl.h>

namespace fs = std::filesystem;

// Utility function to log events in the event log
void AddLog(HWND hwnd, const std::string& logMessage)
{
    HWND hLog = GetDlgItem(hwnd, 2); // Get the event log edit control
    SendMessage(hLog, EM_REPLACESEL, 0, (LPARAM)(logMessage + "\r\n").c_str()); // Append log message
}

// Update the progress bar
void UpdateProgressBar(HWND hwnd, int progress)
{
    HWND hProgressBar = GetDlgItem(hwnd, 5); // Get the progress bar control
    SendMessage(hProgressBar, PBM_SETPOS, progress, 0); // Update progress
}

void ProcessPhpFile(const std::string& filePath, const std::string& htmlDir, HWND hwnd,
    const std::string& productName, const std::string& classesToKeep, const std::string& replaceDir)
{
    std::string newFilePath = htmlDir + filePath.substr(filePath.find_last_of("\\/"));
    newFilePath = newFilePath.substr(0, newFilePath.find_last_of('.')) + ".html";
    fs::create_directories(fs::path(newFilePath).parent_path());

    std::ifstream inFile(filePath);
    std::ofstream outFile(newFilePath);

    if (inFile && outFile)
    {
        std::string line;
        bool inPhpBlock = false;
        bool inHtmlCommentBlock = false;
        bool inScriptBlock = false;
        while (std::getline(inFile, line))
        {
            // Handle PHP blocks (multi-line)
            if (line.find("<?php") != std::string::npos)
                inPhpBlock = true;
            if (inPhpBlock)
            {
                if (line.find("?>") != std::string::npos)
                {
                    inPhpBlock = false;
                }
                continue;
            }

            // Handle HTML comment blocks (multi-line)
            if (line.find("<!--") != std::string::npos)
                inHtmlCommentBlock = true;
            if (inHtmlCommentBlock)
            {
                if (line.find("-->") != std::string::npos)
                {
                    inHtmlCommentBlock = false;
                }
                continue;
            }

            // Handle script blocks (multi-line)
            if (line.find("<script") != std::string::npos)
                inScriptBlock = true;
            if (inScriptBlock)
            {
                if (line.find("</script>") != std::string::npos)
                {
                    inScriptBlock = false;
                }
                continue;
            }

            // Remove unwanted divs
            //static const std::regex bgDivRegex("<div class=\\\"bg.* ? \\\">.*?<div class=\\\"content\\\">(.*?)</div></div>", std::regex::dotall);
            //line = std::regex_replace(line, bgDivRegex, "$1");

            //// Remove animated classes and styles
            //static const std::regex animatedClassRegex(" class=\\\"animated.* ? \\\"");
            //line = std::regex_replace(line, animatedClassRegex, "");

            //static const std::regex styleRegex(" style=\\\".* ? \\\"");
            //line = std::regex_replace(line, styleRegex, "");

            // Write modified line
            outFile << line << "\n";
        }

        AddLog(hwnd, "Processed PHP file: " + filePath);
    }
}

void ProcessEmailFile(const std::string& filePath, const std::string& htmlDir, HWND hwnd, const std::string& yourLink)
{
    std::string destFilePath = htmlDir + "\\emails\\" + fs::path(filePath).filename().string();
    fs::create_directories(htmlDir + "\\emails");

    std::ifstream inFile(filePath);
    std::ofstream outFile(destFilePath);

    if (inFile && outFile)
    {
        std::string line;
        bool removeSignature = false;
        while (std::getline(inFile, line))
        {
            // Skip lines containing forbidden phrases
            if (line.find("resell rights") != std::string::npos ||
                line.find("re-sell rights") != std::string::npos ||
                line.find("resale rights") != std::string::npos ||
                line.find("resellable") != std::string::npos ||
                line.find("licensing rights") != std::string::npos)
            {
                continue;
            }

            // Replace links with "YOUR LINK"
            std::regex urlRegex("https://www.supersalesmachine/.com/*");
            line = std::regex_replace(line, urlRegex, yourLink);

            // Remove signature and anything below it
            if (line.find("Best regards") != std::string::npos)
            {
                removeSignature = true;
                break;
            }

            outFile << line << "\n";
        }

        AddLog(hwnd, "Processed email: " + filePath);
    }
}

void ProcessImageFile(const std::string& filePath, const std::string& htmlDir, HWND hwnd)
{
    std::string fileName = fs::path(filePath).filename().string();
    if (fileName.find("_") != 0 && fileName.find("-") != 0 && !fileName.ends_with("-"))
    {
        fs::create_directories(htmlDir);
        fs::copy(filePath, htmlDir + "\\" + fileName, fs::copy_options::overwrite_existing);
        AddLog(hwnd, "Copied image: " + fileName);
    }
}

// Recursive function to scan all files and subdirectories
void ProcessDirectory(const std::string& directory, const std::string& htmlDir, HWND hwnd,
    const std::string& productName, const std::string& classesToKeep, const std::string& replaceDir, const std::string& yourLink)
{
    for (const auto& entry : fs::directory_iterator(directory))
    {
        if (entry.path() == htmlDir)
            continue;
        if (entry.is_directory())
        {
            std::string newHtmlDir = htmlDir + "\\" + entry.path().filename().string();
            ProcessDirectory(entry.path().string(), newHtmlDir, hwnd, productName, classesToKeep, replaceDir, yourLink);
        }
        else if (entry.is_regular_file())
        {
            std::string filePath = entry.path().string();
            std::string extension = entry.path().extension().string();

            if (extension == ".php")
            {
                ProcessPhpFile(filePath, htmlDir, hwnd, productName, classesToKeep, replaceDir);
            }
            else if (filePath.find("broadcast") != std::string::npos)
            {
                ProcessEmailFile(filePath, htmlDir, hwnd, yourLink);
            }
            else if (extension == ".jpg" || extension == ".png")
            {
                ProcessImageFile(filePath, htmlDir, hwnd);
            }
        }
    }
}

// Main StartConversion function with progress bar integration
void StartConversion(HWND hwnd, const std::string& phpDir, const std::string& templateDir, const std::string& htmlDir, const std::string& productName,
    const std::string& classesToKeep, const std::string& replaceDir, bool deleteUncompressedFiles, const std::string& yourLink)
{
    AddLog(hwnd, "Starting conversion process...");

    // Validate php directory
    if (!fs::exists(phpDir) || !fs::is_directory(phpDir))
    {
        AddLog(hwnd, "Error: PHP directory does not exist or is not a directory.");
        return;
    }
    // create new destination directory
    if (fs::exists(htmlDir))
    {
        try
        {
            fs::remove_all(htmlDir);
            AddLog(hwnd, "HTML directory existed. Removed: " + htmlDir);
        }
        catch (const fs::filesystem_error& e)
        {
            AddLog(hwnd, "Error: Failed to remove existing HTML directory. " + std::string(e.what()));
            return;
        }
    }
    try
    {
        fs::create_directories(htmlDir);
        AddLog(hwnd, "HTML directory created: " + htmlDir);
    }
    catch (const fs::filesystem_error& e)
    {
        AddLog(hwnd, "Error: Failed to create HTML directory. " + std::string(e.what()));
        return;
    }

    ProcessDirectory(phpDir, htmlDir, hwnd, productName, classesToKeep, replaceDir, yourLink);

    UpdateProgressBar(hwnd, 100); // Ensure progress bar reaches 100%
    AddLog(hwnd, "Conversion process completed!");
    MessageBox(hwnd, "Conversion process completed!", "Success", MB_OK);
}
