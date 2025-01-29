#include <windows.h>
#include <commctrl.h>
#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include "Resource.h"
#include <shlobj.h> // For SHBrowseForFolder
#include <map>
#include <sstream>
#include <iostream>

#include "convert.h"

// Global variables
HWND hProgressBar, hInputPHPDir, hInputTemplateDir, hInputHTMLDir, hProductName, hClassField, hReplaceDir, hCheckboxDelete, hCreateButton;

// Function prototypes
void CreateControls(HWND);
std::string BrowseFolder(HWND);
void ProcessFiles(const std::string&, const std::string&);

// Function to save configuration settings to config.ini
void SaveConfig(const std::string& filePath, const std::map<std::string, std::string>& config)
{
    std::ofstream configFile(filePath);
    if (!configFile.is_open())
    {
        std::cerr << "Failed to open config.ini for writing." << std::endl;
        return;
    }
    for (const std::pair<const std::string, std::string>& pair : config)
    {
        configFile << pair.first << "=" << pair.second << "\n";
    }
    configFile.close();
}

// Function to load configuration settings from config.ini
std::map<std::string, std::string> LoadConfig(const std::string& filePath)
{
    std::map<std::string, std::string> config = {
        {"phpDir", "D:\\Work\\Upwork\\20250127\\converting"},
        {"templateDir", "D:\\Work\\Upwork\\20250127\\template"},
        {"htmlDir", "D:\\Work\\Upwork\\20250127\\converting\\_HTML RESELLERS"},
        {"productName", "HealthSupplementNewsletters"},
        {"classesToKeep", "staatliches"},
        {"replaceDir", "health"},
        {"yourLink", "https://YourLink"},
        {"deleteUncompressedFiles", "false"}
    };

    std::ifstream configFile(filePath);
    if (!configFile.is_open())
    {
        std::cerr << "Config file not found. Using default settings." << std::endl;
        SaveConfig(filePath, config); // Save default settings if file does not exist
        return config;
    }

    std::string line;
    while (std::getline(configFile, line))
    {
        std::istringstream ss(line);
        std::string key, value;
        if (std::getline(ss, key, '=') && std::getline(ss, value))
        {
            config[key] = value;
        }
    }

    configFile.close();
    return config;
}

// Callback function to set the initial folder
int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    if (uMsg == BFFM_INITIALIZED && lpData)
    {
        // Set the default directory
        SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
    }
    return 0;
}

std::string BrowseForFolder(HWND hwnd, const char* title, const std::string& defaultPath)
{
    char path[MAX_PATH] = { 0 };

    BROWSEINFO bi = { 0 };
    bi.hwndOwner = hwnd;
    bi.lpszTitle = title;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_USENEWUI;
    bi.lpfn = BrowseCallbackProc; // Set the callback function
    bi.lParam = (LPARAM)defaultPath.c_str(); // Pass the default path to the callback

    // Open the dialog
    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    if (pidl != NULL)
    {
        // Get the selected folder path
        SHGetPathFromIDList(pidl, path);
        CoTaskMemFree(pidl); // Free the memory allocated by SHBrowseForFolder
        return std::string(path);
    }

    return ""; // Return empty string if canceled
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
        CreateControls(hwnd);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case 3: // Browse button for PHP Directory
            {
                char phpDir[MAX_PATH];
                GetWindowText(hInputPHPDir, phpDir, MAX_PATH);
                std::string selectedDir = BrowseForFolder(hwnd, "Select PHP Directory", phpDir);
                if (!selectedDir.empty())
                {
                    SetWindowText(hInputPHPDir, selectedDir.c_str());
                    std::string htmlDir = selectedDir + "\\_HTML RESELLERS";
                    SetWindowText(hInputHTMLDir, htmlDir.c_str());
                }
            }
            break;

        case 11: // Browse button for template Directory
            {
                char templateDir[MAX_PATH];
                GetWindowText(hInputTemplateDir, templateDir, MAX_PATH);
                std::string selectedDir = BrowseForFolder(hwnd, "Select Template Directory", templateDir);
                if (!selectedDir.empty())
                {
                    SetWindowText(hInputTemplateDir, selectedDir.c_str());
                }
            }
            break;

        case 4: // Browse button for HTML Directory
            {
                char htmlDir[MAX_PATH];
                GetWindowText(hInputHTMLDir, htmlDir, MAX_PATH);
                std::string selectedDir = BrowseForFolder(hwnd, "Select HTML Directory", htmlDir);
                if (!selectedDir.empty())
                {
                    SetWindowText(hInputHTMLDir, selectedDir.c_str());
                }
            }
            break;

        case 1: // Create Site button
            {
                char phpDir[MAX_PATH], templateDir[MAX_PATH], htmlDir[MAX_PATH], productName[100], classesToKeep[100], replaceDir[MAX_PATH], yourLink[500];
                GetWindowText(hInputPHPDir, phpDir, MAX_PATH);
                GetWindowText(hInputTemplateDir, templateDir, MAX_PATH);
                GetWindowText(hInputHTMLDir, htmlDir, MAX_PATH);
                GetWindowText(hProductName, productName, 100);
                GetWindowText(hClassField, classesToKeep, 100);
                GetWindowText(hReplaceDir, replaceDir, MAX_PATH);
                GetWindowText(GetDlgItem(hwnd, 6), yourLink, 500); // Get the value of Your Link

                bool deleteUncompressedFiles = SendMessage(hCheckboxDelete, BM_GETCHECK, 0, 0) == BST_CHECKED;
                SendMessage(hProgressBar, PBM_SETPOS, 0, 0);
                SetDlgItemText(hwnd, 2, ""); // Clear the log box

                std::map<std::string, std::string> config = {
                    {"phpDir", phpDir},
                    {"templateDir", templateDir},
                    {"htmlDir", htmlDir},
                    {"productName", productName},
                    {"classesToKeep", classesToKeep},
                    {"replaceDir", replaceDir},
                    {"deleteUncompressedFiles", deleteUncompressedFiles ? "true" : "false"},
                    {"yourLink", yourLink}
                };
                SaveConfig("config.ini", config);

                StartConversion(hwnd, phpDir, templateDir, htmlDir, productName, classesToKeep, replaceDir, deleteUncompressedFiles, yourLink);
            }
            break;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    const char CLASS_NAME[] = "SiteConverter";

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PHP2HTML));
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        "PHP to HTML Site Converter",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 630, 690,
        NULL, NULL, hInstance, NULL
    );

    ShowWindow(hwnd, nCmdShow);

    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

void CreateControls(HWND hwnd)
{
    std::map<std::string, std::string> config = LoadConfig("config.ini");

    std::string phpDir = config["phpDir"];
    std::string templateDir = config["templateDir"];
    std::string htmlDir = config["htmlDir"];
    std::string productName = config["productName"];
    std::string classesToKeep = config["classesToKeep"];
    std::string replaceDir = config["replaceDir"];
    std::string yourLink = config["yourLink"];
    bool deleteUncompressedFiles = (config["deleteUncompressedFiles"] == "true");

    CreateWindow("STATIC", "PHP Directory:", WS_VISIBLE | WS_CHILD, 20, 20, 150, 20, hwnd, NULL, NULL, NULL);
    hInputPHPDir = CreateWindow("EDIT", phpDir.c_str(), WS_VISIBLE | WS_CHILD | WS_BORDER, 160, 20, 400, 20, hwnd, NULL, NULL, NULL);
    CreateWindow("BUTTON", "...", WS_VISIBLE | WS_CHILD, 570, 20, 30, 20, hwnd, (HMENU)3, NULL, NULL);

    CreateWindow("STATIC", "Template Directory:", WS_VISIBLE | WS_CHILD, 20, 60, 150, 20, hwnd, NULL, NULL, NULL);
    hInputTemplateDir = CreateWindow("EDIT", templateDir.c_str(), WS_VISIBLE | WS_CHILD | WS_BORDER, 160, 60, 400, 20, hwnd, NULL, NULL, NULL);
    CreateWindow("BUTTON", "...", WS_VISIBLE | WS_CHILD, 570, 60, 30, 20, hwnd, (HMENU)11, NULL, NULL);

    CreateWindow("STATIC", "HTML Directory:", WS_VISIBLE | WS_CHILD, 20, 100, 150, 20, hwnd, NULL, NULL, NULL);
    hInputHTMLDir = CreateWindow("EDIT", htmlDir.c_str(), WS_VISIBLE | WS_CHILD | WS_BORDER, 160, 100, 400, 20, hwnd, NULL, NULL, NULL);
    CreateWindow("BUTTON", "...", WS_VISIBLE | WS_CHILD, 570, 100, 30, 20, hwnd, (HMENU)4, NULL, NULL);

    CreateWindow("STATIC", "Product Name:", WS_VISIBLE | WS_CHILD, 20, 140, 150, 20, hwnd, NULL, NULL, NULL);
    hProductName = CreateWindow("EDIT", productName.c_str(), WS_VISIBLE | WS_CHILD | WS_BORDER, 160, 140, 400, 20, hwnd, NULL, NULL, NULL);

    CreateWindow("STATIC", "Classes to Keep:", WS_VISIBLE | WS_CHILD, 20, 180, 150, 20, hwnd, NULL, NULL, NULL);
    hClassField = CreateWindow("EDIT", classesToKeep.c_str(), WS_VISIBLE | WS_CHILD | WS_BORDER, 160, 180, 400, 20, hwnd, NULL, NULL, NULL);

    CreateWindow("STATIC", "Replace Directory:", WS_VISIBLE | WS_CHILD, 20, 220, 150, 20, hwnd, NULL, NULL, NULL);
    hReplaceDir = CreateWindow("EDIT", replaceDir.c_str(), WS_VISIBLE | WS_CHILD | WS_BORDER, 160, 220, 400, 20, hwnd, NULL, NULL, NULL);

    CreateWindow("STATIC", "Your Link:", WS_VISIBLE | WS_CHILD, 20, 260, 150, 20, hwnd, NULL, NULL, NULL);
    HWND hYourLink = CreateWindow("EDIT", yourLink.c_str(), WS_VISIBLE | WS_CHILD | WS_BORDER, 160, 260, 400, 20, hwnd, (HMENU)6, NULL, NULL);

    hCheckboxDelete = CreateWindow("BUTTON", "Delete Uncompressed Files", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 20, 300, 200, 20, hwnd, NULL, NULL, NULL);
    if (deleteUncompressedFiles) SendMessage(hCheckboxDelete, BM_SETCHECK, BST_CHECKED, 0);

    hCreateButton = CreateWindow("BUTTON", "Create Site", WS_VISIBLE | WS_CHILD, 250, 340, 100, 30, hwnd, (HMENU)1, NULL, NULL);

    hProgressBar = CreateWindow(PROGRESS_CLASS, NULL, WS_VISIBLE | WS_CHILD, 20, 390, 570, 20, hwnd, (HMENU)5, NULL, NULL);
    SendMessage(hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));

    CreateWindow("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | WS_VSCROLL | ES_READONLY, 20, 430, 570, 200, hwnd, (HMENU)2, NULL, NULL);
}
