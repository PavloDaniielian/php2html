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
        int inBlock = 0;    // 1: comment, 2: php, 3: script, 4: removing div, 5: removing a
        int removeDivTags = 0, skipDivTagsToRemove = 0;
        while (std::getline(inFile, line))
        {
            // process removing block terminal
            switch (inBlock) {
            case 1: // comment block
                if (line.find("-->") != std::string::npos)
                {
                    line = line.substr(line.find("-->") + 3);
                    inBlock = 0;
                    if (line.length() > 0)
                        break;
                }
                continue;
            case 2: // php block
                if (line.find("?>") != std::string::npos)
                {
                    line = line.substr(line.find("?>") + 2);
                    inBlock = 0;
                    if (line.length() > 0)
                        break;
                }
                continue;
            case 3: // script block
                if (line.find("</script>") != std::string::npos)
                {
                    line = line.substr(line.find("</script>") + 9);
                    inBlock = 0;
                    if (line.length() > 0)
                        break;
                }
                continue;
            case 4: // removing div
                if (line.find("<div") != std::string::npos)
                    skipDivTagsToRemove++;
                if (line.find("</div>") != std::string::npos)
                {
                    if (skipDivTagsToRemove > 0)
                        skipDivTagsToRemove--;
                    else
                        inBlock = 0;
                }
                continue;
            case 5: // removing a
                if (line.find("</a>") != std::string::npos)
                    inBlock = 0;
                continue;
            }

            // Remove unwanted divs
            if (line.find("<div class=\"bg") != std::string::npos) {
                removeDivTags ++;
                std::getline(inFile, line);
                if (line.find("<div class=\"content\"") != std::string::npos) {
                    removeDivTags++;
                    std::getline(inFile, line);
                }
            }
            if ( removeDivTags > 0 ){
                if (line.find("<div") != std::string::npos)
                    skipDivTagsToRemove++;
                if (line.find("</div>") != std::string::npos) {
                    if (skipDivTagsToRemove > 0)
                        skipDivTagsToRemove--;
                    else {
                        removeDivTags--;
                        std::getline(inFile, line);
                        continue;
                    }
                }
            }

            // Remove animated classes and styles
            if (line.find("class=\"") != std::string::npos) {
                std::regex replaceRegex("animated");
                line = std::regex_replace(line, replaceRegex, "");
                replaceRegex = "slide-up";
                line = std::regex_replace(line, replaceRegex, "");
                replaceRegex = "slide-down";
                line = std::regex_replace(line, replaceRegex, "");
                replaceRegex = "slide-left";
                line = std::regex_replace(line, replaceRegex, "");
                replaceRegex = "slide-right";
                line = std::regex_replace(line, replaceRegex, "");
                replaceRegex = "zoom";
                line = std::regex_replace(line, replaceRegex, "");
            }
            if (line.find("style=\"") != std::string::npos) {
                std::regex replaceRegex(R"(--speed:\s[0-9]+(\.[0-9]+)?s;?)");
                line = std::regex_replace(line, replaceRegex, "");
            }

            // Replace "PRODUCT NAME" -> "Product Name
            std::regex urlRegex("PRODUCT NAME");
            line = std::regex_replace(line, urlRegex, productName);

            // process removing block start
            if (!inBlock) {
                if (line.find("<!--") != std::string::npos) {
                    size_t t = line.find("-->");
                    if (t == std::string::npos) {
                        line = line.substr(0, line.find("<!--"));
                        inBlock = 1;
                    }else
                        line = line.substr(0, line.find("<!--")) + line.substr(t + 3);
                }else if (line.find("<?php") != std::string::npos) {
                    size_t t = line.find("?>");
                    if (t == std::string::npos){
                        line = line.substr(0, line.find("<?php"));
                        inBlock = 2;
                    }else
                        line = line.substr(0, line.find("<?php")) + line.substr(t + 2);
                }else if (line.find("<script") != std::string::npos) {
                    size_t t = line.find("</script>");
                    if (t == std::string::npos) {
                        line = line.substr(0, line.find("<script"));
                        inBlock = 3;
                    }else
                        line = line.substr(0, line.find("<script")) + line.substr(t + 9);
                }else if (line.find("<div") != std::string::npos && line.find("id=\"noreseller\"") != std::string::npos) {
                    inBlock = 4;
                    line = line.substr(0, line.find("<div"));
                }else if (line.find("<a") != std::string::npos && line.find("href=\"https://warriorplus.com/o2") != std::string::npos) {
                    size_t t = line.find("</a>");
                    if (t == std::string::npos) {
                        line = line.substr(0, line.find("<a"));
                        inBlock = 5;
                    }
                    else
                        line = line.substr(0, line.find("<a")) + line.substr(t + 2);
                }
            }
            // Write modified line
            if (line.length() > 0)
                outFile << line << "\n";
        }

        AddLog(hwnd, "Processed PHP file: " + filePath);
    }
}

void ProcessEmailFile(const std::string& filePath, const std::string& htmlDir, HWND hwnd, const std::string& yourLink)
{
    std::string file = fs::path(filePath).filename().string();
    std::string destFilePath = htmlDir + "\\emails\\" + file.substr(file.find("broadcast"));
    fs::create_directories(htmlDir + "\\emails");

    std::ifstream inFile(filePath);
    std::ofstream outFile(destFilePath);

    if (inFile && outFile)
    {
        std::string line;
        while (std::getline(inFile, line))
        {
            std::string lowerLine = line;
            std::transform(lowerLine.begin(), lowerLine.end(), lowerLine.begin(), ::tolower);
            // Skip lines containing forbidden phrases
            if (lowerLine.find("resell rights") != std::string::npos ||
                lowerLine.find("re-sell rights") != std::string::npos ||
                lowerLine.find("resale rights") != std::string::npos ||
                lowerLine.find("resellable") != std::string::npos ||
                lowerLine.find("licensing rights") != std::string::npos)
            {
                continue;
            }

            // Replace links with "YOUR LINK"
            std::regex urlRegex("https://www.supersalesmachine.com/[^\\s]*");
            lowerLine = std::regex_replace(lowerLine, urlRegex, yourLink);

            // Remove signature and anything below it
            if (lowerLine.find("best regards") != std::string::npos)
            {
                break;
            }

            outFile << lowerLine << "\n";
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
    // remove origin destination directory
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
    // Copy template directory to html directory
    try
    {
        fs::copy(templateDir, htmlDir, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
        AddLog(hwnd, "Copied template directory to template directory: " + htmlDir);
    }
    catch (const fs::filesystem_error& e)
    {
        AddLog(hwnd, "Error: Failed to copy template directory to HTML directory. " + std::string(e.what()));
        return;
    }
    // create new destination directory
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
    // Copy template directory to html directory
    try
    {
        fs::copy(templateDir, htmlDir, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
        AddLog(hwnd, "Copied template directory to template directory: " + htmlDir);
    }
    catch (const fs::filesystem_error& e)
    {
        AddLog(hwnd, "Error: Failed to copy template directory to HTML directory. " + std::string(e.what()));
        return;
    }

    ProcessDirectory(phpDir, htmlDir, hwnd, productName, classesToKeep, replaceDir, yourLink);

    UpdateProgressBar(hwnd, 100); // Ensure progress bar reaches 100%
    AddLog(hwnd, "Conversion process completed!");
    MessageBox(hwnd, "Conversion process completed!", "Success", MB_OK);
}
