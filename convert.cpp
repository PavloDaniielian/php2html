#include "convert.h"
#include "include/zlib.h"


#define PROGRESS_STEP_start     2
#define PROGRESS_STEP_process   30
#define PROGRESS_DURING_process   (PROGRESS_STEP_process-PROGRESS_STEP_start)
#define PROGRESS_STEP_zip       100
#define PROGRESS_DURING_zip       (PROGRESS_STEP_zip-PROGRESS_STEP_process)

std::string trim(std::string s) {
    // Trim leading whitespace
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
        }));

    // Trim trailing whitespace
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
        }).base(), s.end());

    return s;
}

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


int gnAllFilesNum, gnCurFile;
int getFilesNum(const std::string& dir) {
    int fileCount = 0;
    // Iterate through the directory recursively
    for (const auto& entry : fs::recursive_directory_iterator(dir)) {
        if (entry.is_regular_file()) {
            fileCount++;
        }
    }
    return fileCount;
}
int getFilesNumInVector(std::vector<std::string> filesToZip) {
    int fileCount = 0;
    for (const auto& file : filesToZip)
    {
        if (fs::is_directory(file))
        {
            fileCount += getFilesNum(file);
        }
        else if (fs::exists(file))
        {
            fileCount++;
        }
    }
    return fileCount;
}


void ProcessPhpFile(const std::string& filePath, const std::string& newFilePath, int dl, HWND hwnd,
    const std::string& productName, const std::string& classesToKeep, const std::string& replaceDir)
{
    std::string tempFilePath = newFilePath + "_temp";
    bool bInsert = CopyFile(newFilePath.c_str(), tempFilePath.c_str(), FALSE);
    std::ifstream tempFile(tempFilePath);

    std::ifstream inFile(filePath);
    std::ofstream outFile(newFilePath);

    if (inFile && outFile)
    {
        std::string line, prefix;

        // insert head part
        if (bInsert) {
            while (std::getline(tempFile, line) && line.find(STR_PHP_CONTENT_start) == std::string::npos)
                outFile << line << "\n";
            while (std::getline(tempFile, line) && line.find(STR_PHP_CONTENT_end) == std::string::npos);
            while (std::getline(inFile, line) && line.find(STR_PHP_CONTENT_start) == std::string::npos);
        }

        int removeDivTags = 0, skipDivTagsToRemove = 0;
        int oto = 0;
        while (std::getline(inFile, line) && (!bInsert || line.find(STR_PHP_CONTENT_end) == std::string::npos) )
        {
            size_t t = std::string::npos, t1, t2;
            bool removedOnce;
            // remove special block
            do {
                removedOnce = false;
                // remove oto
                t = line.find("if($oto1 == '1')");
                if (t != std::string::npos) {
                    oto = 1;
                    if (dl == 0 || dl == 2) {
                        while (std::getline(inFile, line)) {
                            t = line.find("<?php }");
                            if (t != std::string::npos) {
                                break;
                            }
                        }
                    }
                }
                t = line.find("if($oto2 == '1')");
                if (t != std::string::npos) {
                    oto = 2;
                    if (dl == 0 || dl == 1) {
                        while (std::getline(inFile, line)) {
                            t = line.find("<?php }");
                            if (t != std::string::npos) {
                                break;
                            }
                        }
                    }
                }
                // remove comment block
                t = line.find("<!--");
                if (t != std::string::npos) {
                    prefix = line.substr(0, t);
                    line = line.substr(t + 4);
                    do {
                        t = line.find("-->");
                        if (t != std::string::npos) {
                            line = prefix + line.substr(t + 3);
                            removedOnce = true;
                            break;
                        }
                    } while (std::getline(inFile, line));
                }
                // remove php block
                t = line.find("<?php");
                if (t != std::string::npos) {
                    prefix = line.substr(0, t);
                    line = line.substr(t + 5);
                    do {
                        t = line.find("?>");
                        if (t != std::string::npos) {
                            line = prefix + line.substr(t + 2);
                            removedOnce = true;
                            break;
                        }
                    } while (std::getline(inFile, line));
                }
                // remove script block
                t = line.find("<script");
                if (t != std::string::npos) {
                    prefix = line.substr(0, t);
                    line = line.substr(t + 7);
                    do {
                        t = line.find("</script>");
                        if (t != std::string::npos) {
                            line = prefix + line.substr(t + 9);
                            removedOnce = true;
                            break;
                        }
                    } while (std::getline(inFile, line));
                }
                // remove special div block
                t = line.find("id=\"noreseller\"");
                if (t != std::string::npos) {
                    prefix = line.substr(0, line.find("<div"));
                    line = line.substr(t + 15);
                    while (true) {
                        t1 = line.find("<div");
                        t2 = line.find("</div>");
                        if (t1 < t2) {
                            skipDivTagsToRemove++;
                            line = line.substr(t1 + 5);
                        }
                        else if (t2 < t1) {
                            if (skipDivTagsToRemove > 0) {
                                skipDivTagsToRemove--;
                                line = line.substr(t2 + 6);
                            }
                            else {
                                line = prefix + line.substr(t2 + 6);
                                removedOnce = true;
                                break;
                            }
                        }
                        else
                            std::getline(inFile, line);
                    }
                }
                // remove script a tag
                t = line.find("href=\"https://warriorplus.com/o2\"");
                if (t != std::string::npos) {
                    prefix = line.substr(0, line.find("<a"));
                    line = line.substr(t + 15);
                    do {
                        t = line.find("</a>");
                        if (t != std::string::npos) {
                            line = prefix + line.substr(t + 4);
                            removedOnce = true;
                            break;
                        }
                    } while (std::getline(inFile, line));
                }
            } while (removedOnce);

            // Remove .css? line
            if (line.find(".css?") != std::string::npos)
                continue;

            // Remove unwanted divs
            if (line.find("<div class=\"bg") != std::string::npos) {
                removeDivTags ++;
                std::getline(inFile, line);
                if (line.find("<div class=\"content\"") != std::string::npos) {
                    removeDivTags++;
                    continue;
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
                        continue;
                    }
                }
            }

            std::regex replaceRegex;
            // Remove animated classes and styles
            if (line.find("class=\"") != std::string::npos) {
                replaceRegex = "animated";
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
            replaceRegex = "PRODUCT NAME";
            line = std::regex_replace(line, replaceRegex, productName);

            std::string str_files = oto == 0 ? "files/" : oto == 1 ? "files_oto1/" : "files_oto2/";
            // Replace https://supersalesmachine.s3.amazonaws.com/members/{replaceDir}/ -> files/
            replaceRegex = "https://supersalesmachine.s3.amazonaws.com/members/" + replaceDir + "/";
            line = std::regex_replace(line, replaceRegex, str_files);
            // Replace https://www.supersalesmachine.com/a/{replaceDir}/files/ -> files/
            replaceRegex = "https://www.supersalesmachine.com/a/" + replaceDir + "/files/";
            line = std::regex_replace(line, replaceRegex, str_files);
            // Replace https://www.supersalesmachine.com/o/{replaceDir}/files/ -> files/
            replaceRegex = "https://www.supersalesmachine.com/o/" + replaceDir + "/files/";
            line = std::regex_replace(line, replaceRegex, str_files);

            // Write modified line
            if ( trim(line).length() > 0 )
                outFile << line << "\n";
        }
        inFile.close();

        // insert foot part
        if (bInsert) {
            while (std::getline(tempFile, line) )
                outFile << line << "\n";
            tempFile.close();
            DeleteFile(tempFilePath.c_str());
        }
        outFile.close();

        AddLog(hwnd, "Processed PHP file: " + filePath);
    }
}

void ProcessEmailFile(const std::string& filePath, const std::string& newFilePath, HWND hwnd, const std::string& yourLink)
{
    std::string tempFilePath = newFilePath + "_temp";
    CopyFile(newFilePath.c_str(), tempFilePath.c_str(), FALSE);

    std::ifstream inFile(filePath);
    std::ofstream outFile(newFilePath); // Open in append mode

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

            // Remove signature and anything below it
            if (lowerLine.find("best regards") != std::string::npos)
                break;

            // Replace links with "YOUR LINK"
            std::regex urlRegex("https://www.supersalesmachine.com/[^\\s]*");
            line = std::regex_replace(line, urlRegex, yourLink);

            outFile << line << "\n";
        }
        inFile.close();

        // Append temp content of newFilePath
        std::ifstream tempFile(tempFilePath);
        if (tempFile)
        {
            while (std::getline(tempFile, line))
                outFile << line << "\n";
            tempFile.close();
            DeleteFile(tempFilePath.c_str());
        }
        outFile.close();

        AddLog(hwnd, "Processed email: " + filePath);
    }
}

void ProcessCopyingFile(const std::string& filePath, const std::string& newFilePath, HWND hwnd)
{
    std::string fileName = fs::path(filePath).filename().string();
    if (fileName.find("_") != 0 && fileName.find("-") != 0 && !fileName.ends_with("-"))
    {
        fs::copy(filePath, newFilePath, fs::copy_options::overwrite_existing);
        AddLog(hwnd, "Copied file: " + fileName);
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
        std::string newFile = entry.path().filename().string();
        std::string newFilePath = htmlDir + "\\" + newFile;
        if (entry.is_directory())
        {
            fs::create_directories(newFilePath);
            ProcessDirectory(entry.path().string(), newFilePath, hwnd, productName, classesToKeep, replaceDir, yourLink);
        }
        else if (entry.is_regular_file())
        {
            std::string filePath = entry.path().string();
            std::string extension = entry.path().extension().string();

            if (filePath.find("\\images", directory.length()-7) != std::string::npos) {
                ProcessCopyingFile(filePath, newFilePath, hwnd);
            }
            else if (extension == ".php")
            {
                if (newFile == "dl.php") {
                    newFilePath = htmlDir + "\\thankyou.html";
                    ProcessPhpFile(filePath, newFilePath, 0, hwnd, productName, classesToKeep, replaceDir);
                    newFilePath = htmlDir + "\\thankyou_with_oto1.html";
                    ProcessPhpFile(filePath, newFilePath, 1, hwnd, productName, classesToKeep, replaceDir);
                    newFilePath = htmlDir + "\\thankyou_with_oto2.html";
                    ProcessPhpFile(filePath, newFilePath, 2, hwnd, productName, classesToKeep, replaceDir);
                    newFilePath = htmlDir + "\\thankyou_with_oto1_oto2.html";
                    ProcessPhpFile(filePath, newFilePath, 12, hwnd, productName, classesToKeep, replaceDir);
                }
                else {
                    newFilePath = newFilePath.substr(0, newFilePath.find_last_of('.')) + ".html";
                    ProcessPhpFile(filePath, newFilePath, -1, hwnd, productName, classesToKeep, replaceDir);
                }
            }

            if ( fs::exists(newFilePath) && fs::file_size(newFilePath) == 0)
                DeleteFile(newFilePath.c_str());

            gnCurFile++;
            UpdateProgressBar(hwnd, PROGRESS_STEP_start + ((float)gnCurFile / gnAllFilesNum) * PROGRESS_DURING_process);
        }
    }
}



// Structure to store central directory entries
struct CentralDirectoryEntry {
    std::string fileName;
    uint32_t crc;
    uint32_t compressedSize;
    uint32_t uncompressedSize;
    uint32_t localHeaderOffset;
};

// Write Local File Header
void WriteLocalFileHeader(std::ofstream& zipFile, const std::string& fileName, uint32_t compressedSize, uint32_t crc, bool isDirectory) {
    uint32_t signature = 0x04034b50; // Local file header signature
    uint16_t versionNeededToExtract = 20;
    uint16_t flags = 0;
    uint16_t compression = isDirectory ? 0 : 8; // No compression for directories
    uint16_t modTime = 0;
    uint16_t modDate = 0;
    uint16_t fileNameLength = static_cast<uint16_t>(fileName.size());
    uint16_t extraFieldLength = 0;

    zipFile.write(reinterpret_cast<const char*>(&signature), sizeof(signature));
    zipFile.write(reinterpret_cast<const char*>(&versionNeededToExtract), sizeof(versionNeededToExtract));
    zipFile.write(reinterpret_cast<const char*>(&flags), sizeof(flags));
    zipFile.write(reinterpret_cast<const char*>(&compression), sizeof(compression));
    zipFile.write(reinterpret_cast<const char*>(&modTime), sizeof(modTime));
    zipFile.write(reinterpret_cast<const char*>(&modDate), sizeof(modDate));
    zipFile.write(reinterpret_cast<const char*>(&crc), sizeof(crc));
    zipFile.write(reinterpret_cast<const char*>(&compressedSize), sizeof(compressedSize));
    zipFile.write(reinterpret_cast<const char*>(&compressedSize), sizeof(compressedSize)); // Same as uncompressed size for directories
    zipFile.write(reinterpret_cast<const char*>(&fileNameLength), sizeof(fileNameLength));
    zipFile.write(reinterpret_cast<const char*>(&extraFieldLength), sizeof(extraFieldLength));
    zipFile.write(fileName.c_str(), fileNameLength);
}

// Write Central Directory
void WriteCentralDirectory(std::ofstream& zipFile, const std::vector<CentralDirectoryEntry>& centralDirectory, uint32_t centralDirectoryOffset) {
    uint32_t centralDirectorySize = 0;

    for (const auto& entry : centralDirectory) {
        uint32_t signature = 0x02014b50; // Central directory file header signature
        uint16_t versionMadeBy = 20;
        uint16_t versionNeededToExtract = 20;
        uint16_t flags = 0;
        uint16_t compression = entry.compressedSize == 0 ? 0 : 8; // No compression for directories
        uint16_t modTime = 0;
        uint16_t modDate = 0;
        uint16_t fileNameLength = static_cast<uint16_t>(entry.fileName.size());
        uint16_t extraFieldLength = 0;
        uint16_t fileCommentLength = 0;
        uint16_t diskNumberStart = 0;
        uint16_t internalFileAttributes = 0;
        uint32_t externalFileAttributes = entry.compressedSize == 0 ? 0x10 : 0; // Directory flag for external attributes
        uint32_t localHeaderOffset = entry.localHeaderOffset;

        zipFile.write(reinterpret_cast<const char*>(&signature), sizeof(signature));
        zipFile.write(reinterpret_cast<const char*>(&versionMadeBy), sizeof(versionMadeBy));
        zipFile.write(reinterpret_cast<const char*>(&versionNeededToExtract), sizeof(versionNeededToExtract));
        zipFile.write(reinterpret_cast<const char*>(&flags), sizeof(flags));
        zipFile.write(reinterpret_cast<const char*>(&compression), sizeof(compression));
        zipFile.write(reinterpret_cast<const char*>(&modTime), sizeof(modTime));
        zipFile.write(reinterpret_cast<const char*>(&modDate), sizeof(modDate));
        zipFile.write(reinterpret_cast<const char*>(&entry.crc), sizeof(entry.crc));
        zipFile.write(reinterpret_cast<const char*>(&entry.compressedSize), sizeof(entry.compressedSize));
        zipFile.write(reinterpret_cast<const char*>(&entry.uncompressedSize), sizeof(entry.uncompressedSize));
        zipFile.write(reinterpret_cast<const char*>(&fileNameLength), sizeof(fileNameLength));
        zipFile.write(reinterpret_cast<const char*>(&extraFieldLength), sizeof(extraFieldLength));
        zipFile.write(reinterpret_cast<const char*>(&fileCommentLength), sizeof(fileCommentLength));
        zipFile.write(reinterpret_cast<const char*>(&diskNumberStart), sizeof(diskNumberStart));
        zipFile.write(reinterpret_cast<const char*>(&internalFileAttributes), sizeof(internalFileAttributes));
        zipFile.write(reinterpret_cast<const char*>(&externalFileAttributes), sizeof(externalFileAttributes));
        zipFile.write(reinterpret_cast<const char*>(&localHeaderOffset), sizeof(localHeaderOffset));
        zipFile.write(entry.fileName.c_str(), fileNameLength);

        centralDirectorySize += sizeof(signature) + 46 + fileNameLength;
    }

    // Write End of Central Directory (EOCD) record
    uint32_t eocdSignature = 0x06054b50;
    uint16_t numberOfDisk = 0;
    uint16_t centralDirectoryDisk = 0;
    uint16_t centralDirectoryEntries = static_cast<uint16_t>(centralDirectory.size());
    uint16_t totalEntries = static_cast<uint16_t>(centralDirectory.size());
    uint32_t centralDirectorySizeField = centralDirectorySize;
    uint32_t centralDirectoryOffsetField = centralDirectoryOffset;
    uint16_t commentLength = 0;

    zipFile.write(reinterpret_cast<const char*>(&eocdSignature), sizeof(eocdSignature));
    zipFile.write(reinterpret_cast<const char*>(&numberOfDisk), sizeof(numberOfDisk));
    zipFile.write(reinterpret_cast<const char*>(&centralDirectoryDisk), sizeof(centralDirectoryDisk));
    zipFile.write(reinterpret_cast<const char*>(&centralDirectoryEntries), sizeof(centralDirectoryEntries));
    zipFile.write(reinterpret_cast<const char*>(&totalEntries), sizeof(totalEntries));
    zipFile.write(reinterpret_cast<const char*>(&centralDirectorySizeField), sizeof(centralDirectorySizeField));
    zipFile.write(reinterpret_cast<const char*>(&centralDirectoryOffsetField), sizeof(centralDirectoryOffsetField));
    zipFile.write(reinterpret_cast<const char*>(&commentLength), sizeof(commentLength));
}


// Recursively compress a directory and its contents
void CompressDirectory(std::string zipFileName, const std::string& dirPath, std::ofstream& zipFile, std::vector<CentralDirectoryEntry>& centralDirectory, uint32_t& localHeaderOffset, HWND hwnd, const std::string& basePath)
{
    for (const auto& entry : fs::directory_iterator(dirPath))
    {
        std::string relativePath = basePath + fs::path(entry.path()).filename().string();
        if (entry.is_directory())
        {
            relativePath += "/"; // Ensure directory names end with a slash
            WriteLocalFileHeader(zipFile, relativePath, 0, 0, true);
            centralDirectory.push_back({ relativePath, 0, 0, 0, localHeaderOffset });
            localHeaderOffset += 30 + relativePath.size();

            //AddLog(hwnd, zipPath + ": " + "Added directory: " + relativePath);

            // Recursively process the directory
            CompressDirectory(zipFileName, entry.path().string(), zipFile, centralDirectory, localHeaderOffset, hwnd, relativePath);
        }
        else if (entry.is_regular_file())
        {
            gnCurFile++;
            UpdateProgressBar(hwnd, PROGRESS_STEP_process + ((float)gnCurFile / gnAllFilesNum) * PROGRESS_DURING_zip);

            std::ifstream inputFile(entry.path().string(), std::ios::binary);
            if (!inputFile.is_open())
            {
                AddLog(hwnd, zipFileName + ": " + "Failed to open file: " + entry.path().string());
                continue;
            }

            std::vector<char> sourceData((std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>());
            inputFile.close();

            uLongf compressedSize = compressBound(sourceData.size());
            std::vector<Bytef> compressedData(compressedSize);
            uLong crc = crc32(0L, Z_NULL, 0);
            crc = crc32(crc, reinterpret_cast<const Bytef*>(sourceData.data()), sourceData.size());
            int result = compress(compressedData.data(), &compressedSize, reinterpret_cast<const Bytef*>(sourceData.data()), sourceData.size());
            if (result != Z_OK)
            {
                AddLog(hwnd, zipFileName + ": " + "Compression failed for file: " + entry.path().string());
                continue;
            }

            WriteLocalFileHeader(zipFile, relativePath, compressedSize, crc, false);
            zipFile.write(reinterpret_cast<const char*>(compressedData.data()), compressedSize);

            centralDirectory.push_back({ relativePath, crc, static_cast<uint32_t>(compressedSize), static_cast<uint32_t>(sourceData.size()), localHeaderOffset });
            localHeaderOffset += 30 + relativePath.size() + compressedSize;

            AddLog(hwnd, zipFileName + ": " + "Added file: " + relativePath);
        }
    }
}

// Updated CreateZip Function
void CreateZip(const std::string& zipPath, const std::vector<std::string>& filesToZip, HWND hwnd)
{
    std::string zipFileName = zipPath.substr(zipPath.find_last_of("\\") + 1);

    AddLog(hwnd, "Creating ZIP archive: " + zipPath);

    std::ofstream zipFile(zipPath, std::ios::binary);
    if (!zipFile.is_open())
    {
        AddLog(hwnd, "Failed to create ZIP file: " + zipPath);
        return;
    }

    std::vector<CentralDirectoryEntry> centralDirectory;
    uint32_t localHeaderOffset = 0;

    for (const auto& file : filesToZip)
    {
        if (fs::is_directory(file))
        {
            AddLog(hwnd, zipFileName + ": " + "Compressing directory: " + file);
            CompressDirectory(zipFileName, file, zipFile, centralDirectory, localHeaderOffset, hwnd, fs::path(file).filename().string() + "/");
        }
        else if (fs::exists(file))
        {
            std::ifstream inputFile(file, std::ios::binary);
            if (!inputFile.is_open())
            {
                AddLog(hwnd, zipFileName + ": " + "Failed to open file: " + file);
                continue;
            }

            std::vector<char> sourceData((std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>());
            inputFile.close();

            uLongf compressedSize = compressBound(sourceData.size());
            std::vector<Bytef> compressedData(compressedSize);
            uLong crc = crc32(0L, Z_NULL, 0);
            crc = crc32(crc, reinterpret_cast<const Bytef*>(sourceData.data()), sourceData.size());
            int result = compress(compressedData.data(), &compressedSize, reinterpret_cast<const Bytef*>(sourceData.data()), sourceData.size());
            if (result != Z_OK)
            {
                AddLog(hwnd, zipFileName + ": " + "Compression failed for file: " + file);
                continue;
            }

            std::string fileName = fs::path(file).filename().string();
            WriteLocalFileHeader(zipFile, fileName, compressedSize, crc, false);
            zipFile.write(reinterpret_cast<const char*>(compressedData.data()), compressedSize);

            centralDirectory.push_back({ fileName, crc, static_cast<uint32_t>(compressedSize), static_cast<uint32_t>(sourceData.size()), localHeaderOffset });
            localHeaderOffset += 30 + fileName.size() + compressedSize;

            AddLog(hwnd, zipFileName + ": " + "Added file: " + fileName);
        }
        else
        {
            AddLog(hwnd, zipFileName + ": " + "File or directory not found: " + file);
        }
    }

    uint32_t centralDirectoryOffset = localHeaderOffset;
    WriteCentralDirectory(zipFile, centralDirectory, centralDirectoryOffset);

    zipFile.close();
    AddLog(hwnd, "ZIP archive created successfully: " + zipPath);
}

void DeleteUncompressedFiles(const std::string& dir) {
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.is_directory()) {
            // Recursively delete the subdirectory
            DeleteUncompressedFiles(entry.path().string());
            // Remove the empty directory after processing its contents
            fs::remove(entry.path());
        }
        else {
            // Check if the file is not a .zip file
            if (entry.path().extension() != ".zip") {
                fs::remove(entry.path());
            }
        }
    }
}


// Main StartConversion function with progress bar integration
void StartConversion(HWND hwnd, const std::string& phpDir, const std::string& templateDir, const std::string& htmlDir, const std::string& productName,
    const std::string& classesToKeep, const std::string& replaceDir, std::map<std::string, std::string>  emailMap, const std::string& yourLink, bool deleteUncompressedFiles)
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

    gnAllFilesNum = getFilesNum(phpDir);

    UpdateProgressBar(hwnd, PROGRESS_STEP_start);

    gnCurFile = 0;
    // process emails
    for (const auto& email : emailMap)
    {
        std::string src = phpDir + "\\" + email.first;
        std::string dst = htmlDir + "\\emails\\" + email.second;
        fs::create_directories(fs::path(dst).parent_path());
        ProcessEmailFile(src, dst, hwnd, yourLink);
        gnCurFile++;
        UpdateProgressBar(hwnd, PROGRESS_STEP_start + ((float)gnCurFile / gnAllFilesNum) * PROGRESS_DURING_process);
    }

    // process other files
    ProcessDirectory(phpDir, htmlDir, hwnd, productName, classesToKeep, replaceDir, yourLink);
    UpdateProgressBar(hwnd, PROGRESS_STEP_process);

    // zip up
    // Files to zip
    std::vector<std::string> filesToZipBase = {
        htmlDir + "\\disclaimer.html",
        htmlDir + "\\emails",
        htmlDir + "\\files",
        htmlDir + "\\images",
        htmlDir + "\\index.html",
        htmlDir + "\\js",
        htmlDir + "\\privacy.html",
        htmlDir + "\\terms.html",
        htmlDir + "\\thankyou.html",
        htmlDir + "\\thankyou_signup.html",
        htmlDir + "\\affiliates",
        htmlDir + "\\affiliates.html",
        htmlDir + "\\jv",
        htmlDir + "\\jv.html",
    };
    std::vector<std::string> filesToZip, filesToZip1, filesToZip2, filesToZip12;
    std::string productNameWithoutSpaces = productName;
    productNameWithoutSpaces.erase(std::remove_if(productNameWithoutSpaces.begin(), productNameWithoutSpaces.end(), ::isspace), productNameWithoutSpaces.end());
    
    // calculate all files number to zipup
    gnAllFilesNum = 0;
    // RR
    {
        filesToZip = filesToZipBase;
        gnAllFilesNum += getFilesNumInVector(filesToZip);
    }
    // RR1
    if (fs::exists(phpDir + "\\oto1.php")) {
        filesToZip1 = filesToZipBase;
        filesToZip1.push_back(htmlDir + "\\files_oto1");
        filesToZip1.push_back(htmlDir + "\\images_oto1");
        filesToZip1.push_back(htmlDir + "\\oto1.html");
        filesToZip1.push_back(htmlDir + "\\thankyou_with_oto1.html");
        gnAllFilesNum += getFilesNumInVector(filesToZip1);
    }
    // RR2
    if (fs::exists(phpDir + "\\oto2.php")) {
        filesToZip2 = filesToZipBase;
        filesToZip2.push_back(htmlDir + "\\files_oto2");
        filesToZip2.push_back(htmlDir + "\\images_oto2");
        filesToZip2.push_back(htmlDir + "\\oto2.html");
        filesToZip2.push_back(htmlDir + "\\thankyou_with_oto2.html");
        gnAllFilesNum += getFilesNumInVector(filesToZip2);
    }
    // RR12
    if (fs::exists(phpDir + "\\oto1.php") && fs::exists(phpDir + "\\oto2.php")) {
        filesToZip12 = filesToZip1;
        filesToZip12.push_back(htmlDir + "\\files_oto2");
        filesToZip12.push_back(htmlDir + "\\images_oto2");
        filesToZip12.push_back(htmlDir + "\\oto2.html");
        filesToZip12.push_back(htmlDir + "\\thankyou_with_oto2.html");
        gnAllFilesNum += getFilesNumInVector(filesToZip12);
    }
    gnCurFile = 0;

    // RR
    {
        std::string zipPath = htmlDir + "\\" + productNameWithoutSpaces + "_RR.zip";
        CreateZip(zipPath, filesToZip, hwnd);
    }
    // RR1
    if (fs::exists(phpDir + "\\oto1.php")) {
        std::string zipPath = htmlDir + "\\" + productNameWithoutSpaces + "_RROTO1.zip";
        CreateZip(zipPath, filesToZip1, hwnd);
    }
    // RR2
    if (fs::exists(phpDir + "\\oto2.php")) {
        std::string zipPath = htmlDir + "\\" + productNameWithoutSpaces + "_RROTO2.zip";
        CreateZip(zipPath, filesToZip2, hwnd);
    }
    // RR12
    if (fs::exists(phpDir + "\\oto2.php")) {
        std::string zipPath = htmlDir + "\\" + productNameWithoutSpaces + "_RROTO12.zip";
        CreateZip(zipPath, filesToZip12, hwnd);
    }

    // delete uncompressed files
    if (deleteUncompressedFiles) {
        DeleteUncompressedFiles(htmlDir);
        AddLog(hwnd, "Deleted uncompressed files!");
    }

    UpdateProgressBar(hwnd, 100); // Ensure progress bar reaches 100%
    AddLog(hwnd, "Conversion process completed!");
    MessageBox(hwnd, "Conversion process completed!", "Success", MB_OK);
}
