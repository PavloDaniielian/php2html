#include <windows.h>
#include "Resource.h"
#include <vector>
#include <shlobj.h> // For SHBrowseForFolder
#include <sstream>
#include <iostream>

#include "convert.h"

// Global variables
HWND hProgressBar, hInputPHPDir, hInputTemplateDir, hInputHTMLDir, hProductName, hClassField, hReplaceDir, hCheckboxDelete, hCreateButton, hYourLink;
#define MY_MAX_PATH 1000

// Function prototypes
void CreateControls(HWND);
void ResizeControls(HWND hwnd, int w, int h);
void DetectEmails(HWND hwnd);

std::string BrowseFolder(HWND);
void ProcessFiles(const std::string&, const std::string&);

// Function to save configuration settings to setting.ini
void SaveConfig(const std::string& filePath, const std::map<std::string, std::string>& config)
{
    std::ofstream configFile(filePath);
    if (!configFile.is_open())
    {
        std::cerr << "Failed to open setting.ini for writing." << std::endl;
        return;
    }
    for (const std::pair<const std::string, std::string>& pair : config)
    {
        configFile << pair.first << "=" << pair.second << "\n";
    }
    configFile.close();
}

// Function to load configuration settings from setting.ini
std::map<std::string, std::string> LoadConfig(const std::string& filePath)
{
    std::map<std::string, std::string> config = {
        {"phpDir", "D:\\Work\\Upwork\\20250127\\converting"},
        {"templateDir", "D:\\Work\\Upwork\\20250127\\template"},
        {"htmlDir", "D:\\Work\\Upwork\\20250127\\converting\\_HTML RESELLERS"},
        {"productName", "HealthSupplementNewsletters"},
        {"classesToKeep", "staatliches"},
        {"replaceDir", "health"},
        {"yourLink", "YOUR LINK"},
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

// Function to open a modern folder dialog with default directory
std::string OpenFolderDialog(HWND hwnd, const std::string& title, const std::string& defaultPath)
{
    std::string selectedPath;
    IFileDialog* pFileDialog = nullptr;

    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_IFileDialog, reinterpret_cast<void**>(&pFileDialog));
    if (FAILED(hr)) return ""; // Return empty string if failed

    // Set folder selection mode
    DWORD dwOptions;
    pFileDialog->GetOptions(&dwOptions);
    pFileDialog->SetOptions(dwOptions | FOS_PICKFOLDERS);

    // Set dialog title
    pFileDialog->SetTitle(std::wstring(title.begin(), title.end()).c_str());

    // Set default directory if provided
    if (!defaultPath.empty())
    {
        IShellItem* pDefaultFolder = nullptr;
        std::wstring wDefaultPath(defaultPath.begin(), defaultPath.end());
        hr = SHCreateItemFromParsingName(wDefaultPath.c_str(), NULL, IID_PPV_ARGS(&pDefaultFolder));
        if (SUCCEEDED(hr))
        {
            pFileDialog->SetFolder(pDefaultFolder);
            pDefaultFolder->Release();
        }
    }

    // Show dialog
    hr = pFileDialog->Show(hwnd);
    if (SUCCEEDED(hr))
    {
        // Get the selected folder
        IShellItem* pItem = nullptr;
        hr = pFileDialog->GetResult(&pItem);
        if (SUCCEEDED(hr))
        {
            PWSTR pszPath;
            hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
            if (SUCCEEDED(hr))
            {
                //selectedPath = std::string(std::wstring(pszPath).begin(), std::wstring(pszPath).end());
                char s[MY_MAX_PATH];
                WideCharToMultiByte(CP_ACP, 0, pszPath, MY_MAX_PATH, s, MY_MAX_PATH, 0, 0);
                selectedPath = std::string(s);
                CoTaskMemFree(pszPath);
            }
            pItem->Release();
        }
    }

    pFileDialog->Release();
    return selectedPath;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
        CreateControls(hwnd);
        DetectEmails(hwnd);
        break;
    case WM_SIZE:
        ResizeControls(hwnd, LOWORD(lParam), HIWORD(lParam));
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case 3: // Browse button for PHP Directory
            {
                char phpDir[MY_MAX_PATH];
                GetWindowText(hInputPHPDir, phpDir, MY_MAX_PATH);
                std::string selectedDir = OpenFolderDialog(hwnd, "Select PHP Directory", phpDir);
                if (!selectedDir.empty())
                {
                    SetWindowText(hInputPHPDir, selectedDir.c_str());
                    std::string htmlDir = selectedDir + "\\_HTML RESELLERS";
                    SetWindowText(hInputHTMLDir, htmlDir.c_str());
                    DetectEmails(hwnd);
                }
            }
            break;

        case 11: // Browse button for template Directory
            {
                char templateDir[MY_MAX_PATH];
                GetWindowText(hInputTemplateDir, templateDir, MY_MAX_PATH);
                std::string selectedDir = OpenFolderDialog(hwnd, "Select Template Directory", templateDir);
                if (!selectedDir.empty())
                {
                    SetWindowText(hInputTemplateDir, selectedDir.c_str());
                }
            }
            break;

        case 4: // Browse button for HTML Directory
            {
                char htmlDir[MY_MAX_PATH];
                GetWindowText(hInputHTMLDir, htmlDir, MY_MAX_PATH);
                std::string selectedDir = OpenFolderDialog(hwnd, "Select HTML Directory", htmlDir);
                if (!selectedDir.empty())
                {
                    SetWindowText(hInputHTMLDir, selectedDir.c_str());
                }
            }
            break;

        case 1: // Create Site button
            {
                char phpDir[MY_MAX_PATH], templateDir[MY_MAX_PATH], htmlDir[MY_MAX_PATH], productName[100], classesToKeep[100], replaceDir[MY_MAX_PATH], yourLink[500];
                GetWindowText(hInputPHPDir, phpDir, MY_MAX_PATH);
                GetWindowText(hInputTemplateDir, templateDir, MY_MAX_PATH);
                GetWindowText(hInputHTMLDir, htmlDir, MY_MAX_PATH);
                GetWindowText(hProductName, productName, 100);
                GetWindowText(hClassField, classesToKeep, 100);
                GetWindowText(hReplaceDir, replaceDir, MY_MAX_PATH);

                std::map<std::string, std::string>  emailMap;
                for (int i = 0; i < MAX_EMAILS_NUM; i++) {
                    char _in[MAX_PATH], _out[MAX_PATH];
                    GetWindowText(GetDlgItem(hwnd, 1010 + 10 * i + 2), _in, MAX_PATH);
                    if (strlen(_in) <= 0)
                        continue;
                    GetWindowText(GetDlgItem(hwnd, 1010 + 10 * i + 4), _out, MAX_PATH);
                    if (strlen(_out) <= 0 || !strcmp(_out, "(none)"))
                        continue;
                    emailMap.insert({ _in, _out });
                }

                GetWindowText(hYourLink, yourLink, 500); // Get the value of Your Link

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
                SaveConfig("setting.ini", config);

                StartConversion(hwnd, phpDir, templateDir, htmlDir, productName, classesToKeep, replaceDir, emailMap, yourLink, deleteUncompressedFiles);
            }
            break;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
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
        CW_USEDEFAULT, CW_USEDEFAULT, 1000, 950,
        NULL, NULL, hInstance, NULL
    );

    CoInitialize(NULL);

    ShowWindow(hwnd, nCmdShow);

    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

LRESULT CALLBACK EditSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    if (uMsg == WM_KEYDOWN)
    {
        if (wParam == 'A' && (GetKeyState(VK_CONTROL) & 0x8000))
        {
            SendMessage(hWnd, EM_SETSEL, 0, -1); // Select all text
            return 0; // Mark as handled
        }
    }

    // Call the default window procedure for other messages
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void CreateControls(HWND hwnd)
{
    std::map<std::string, std::string> config = LoadConfig("setting.ini");

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
    SetWindowSubclass(hInputPHPDir, EditSubclassProc, 0, 0); // Apply subclassing
    CreateWindow("BUTTON", "...", WS_VISIBLE | WS_CHILD, 570, 20, 30, 20, hwnd, (HMENU)3, NULL, NULL);

    CreateWindow("STATIC", "Template Directory:", WS_VISIBLE | WS_CHILD, 20, 60, 150, 20, hwnd, NULL, NULL, NULL);
    hInputTemplateDir = CreateWindow("EDIT", templateDir.c_str(), WS_VISIBLE | WS_CHILD | WS_BORDER, 160, 60, 400, 20, hwnd, NULL, NULL, NULL);
    SetWindowSubclass(hInputTemplateDir, EditSubclassProc, 0, 0); // Apply subclassing
    CreateWindow("BUTTON", "...", WS_VISIBLE | WS_CHILD, 570, 60, 30, 20, hwnd, (HMENU)11, NULL, NULL);

    CreateWindow("STATIC", "HTML Directory:", WS_VISIBLE | WS_CHILD, 20, 100, 150, 20, hwnd, NULL, NULL, NULL);
    hInputHTMLDir = CreateWindow("EDIT", htmlDir.c_str(), WS_VISIBLE | WS_CHILD | WS_BORDER, 160, 100, 400, 20, hwnd, NULL, NULL, NULL);
    SetWindowSubclass(hInputHTMLDir, EditSubclassProc, 0, 0); // Apply subclassing
    CreateWindow("BUTTON", "...", WS_VISIBLE | WS_CHILD, 570, 100, 30, 20, hwnd, (HMENU)4, NULL, NULL);

    CreateWindow("STATIC", "Product Name:", WS_VISIBLE | WS_CHILD, 20, 140, 150, 20, hwnd, NULL, NULL, NULL);
    hProductName = CreateWindow("EDIT", productName.c_str(), WS_VISIBLE | WS_CHILD | WS_BORDER, 160, 140, 400, 20, hwnd, NULL, NULL, NULL);
    SetWindowSubclass(hProductName, EditSubclassProc, 0, 0); // Apply subclassing

    CreateWindow("STATIC", "Classes to Keep:", WS_VISIBLE | WS_CHILD, 20, 180, 150, 20, hwnd, NULL, NULL, NULL);
    hClassField = CreateWindow("EDIT", classesToKeep.c_str(), WS_VISIBLE | WS_CHILD | WS_BORDER, 160, 180, 400, 20, hwnd, NULL, NULL, NULL);
    SetWindowSubclass(hClassField, EditSubclassProc, 0, 0); // Apply subclassing

    CreateWindow("STATIC", "Replace Directory:", WS_VISIBLE | WS_CHILD, 20, 220, 150, 20, hwnd, NULL, NULL, NULL);
    hReplaceDir = CreateWindow("EDIT", replaceDir.c_str(), WS_VISIBLE | WS_CHILD | WS_BORDER, 160, 220, 400, 20, hwnd, NULL, NULL, NULL);
    SetWindowSubclass(hReplaceDir, EditSubclassProc, 0, 0); // Apply subclassing

    CreateWindow("STATIC", "Assign Emails:", WS_VISIBLE | WS_CHILD, 20, 180, 150, 20, hwnd, (HMENU)1000, NULL, NULL);
    CreateWindow("STATIC", "Source Site Emails (detected)", WS_VISIBLE | WS_CHILD, 20, 180, 150, 20, hwnd, (HMENU)1001, NULL, NULL);
    CreateWindow("STATIC", "Client's Destination Emails (emails/*.txt)", WS_VISIBLE | WS_CHILD, 20, 180, 150, 20, hwnd, (HMENU)1002, NULL, NULL);
    for (int i = 0; i < MAX_EMAILS_NUM; ++i)
    {
        std::string label = "Email " + std::to_string(i + 1) + ":";
        CreateWindow("STATIC", label.c_str(), WS_VISIBLE | WS_CHILD, 20, 290 + (i * 30), 100, 20, hwnd, (HMENU)(1010 + i * 10 + 1), NULL, NULL);
        CreateWindow("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER, 130, 290 + (i * 30), 200, 20, hwnd, (HMENU)(1010 + i * 10 + 2), NULL, NULL);
        CreateWindow("STATIC", (std::string("    =====>    ") + label).c_str(), WS_VISIBLE | WS_CHILD, 340, 290 + (i * 30), 20, 20, hwnd, (HMENU)(1010 + i * 10 + 3), NULL, NULL);
        CreateWindow("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER, 370, 290 + (i * 30), 200, 20, hwnd, (HMENU)(1010 + i * 10 + 4), NULL, NULL);
    }

    CreateWindow("STATIC", "Email Links:", WS_VISIBLE | WS_CHILD, 20, 260, 150, 20, hwnd, (HMENU)12, NULL, NULL);
    hYourLink = CreateWindow("EDIT", yourLink.c_str(), WS_VISIBLE | WS_CHILD | WS_BORDER, 160, 260, 400, 20, hwnd, NULL, NULL, NULL);
    SetWindowSubclass(hYourLink, EditSubclassProc, 0, 0); // Apply subclassing

    hCheckboxDelete = CreateWindow("BUTTON", "Delete Uncompressed Files", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 20, 300, 200, 20, hwnd, NULL, NULL, NULL);
    if (deleteUncompressedFiles) SendMessage(hCheckboxDelete, BM_SETCHECK, BST_CHECKED, 0);

    hCreateButton = CreateWindow("BUTTON", "Create Site", WS_VISIBLE | WS_CHILD, 250, 340, 100, 30, hwnd, (HMENU)1, NULL, NULL);

    hProgressBar = CreateWindow(PROGRESS_CLASS, NULL, WS_VISIBLE | WS_CHILD, 20, 390, 570, 20, hwnd, (HMENU)5, NULL, NULL);
    SendMessage(hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));

    HWND hLog = CreateWindow("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | WS_VSCROLL | ES_READONLY, 20, 430, 570, 200, hwnd, (HMENU)2, NULL, NULL);
    SendMessage(hLog, EM_LIMITTEXT, (WPARAM)-1, 0);
}

void ResizeControls(HWND hwnd, int w, int h)
{
    SetWindowPos(hInputPHPDir, NULL, 160, 20, w-160 - 20 - 30, 20, SWP_NOZORDER);
    SetWindowPos(GetDlgItem(hwnd, 3), NULL, w - 15 -30, 20, 30, 20, SWP_NOZORDER);

    SetWindowPos(hInputTemplateDir, NULL, 160, 60, w - 160 - 20 - 30, 20, SWP_NOZORDER);
    SetWindowPos(GetDlgItem(hwnd, 11), NULL, w - 15 - 30, 60, 30, 20, SWP_NOZORDER);

    SetWindowPos(hInputHTMLDir, NULL, 160, 100, w - 160 - 20 - 30, 20, SWP_NOZORDER);
    SetWindowPos(GetDlgItem(hwnd, 4), NULL, w - 15 - 30, 100, 30, 20, SWP_NOZORDER);

    SetWindowPos(hProductName, NULL, 160, 140, w - 160 - 15, 20, SWP_NOZORDER);
    SetWindowPos(hClassField, NULL, 160, 180, w - 160 - 15, 20, SWP_NOZORDER);
    SetWindowPos(hReplaceDir, NULL, 160, 220, w - 160 - 15, 20, SWP_NOZORDER);

    SetWindowPos(GetDlgItem(hwnd, 1000), NULL, 20, 260, 150, 20, SWP_NOZORDER);
    SetWindowPos(GetDlgItem(hwnd, 1001), NULL, 160, 260, 300, 20, SWP_NOZORDER);
    SetWindowPos(GetDlgItem(hwnd, 1002), NULL, w/2 + 160, 260, 300, 20, SWP_NOZORDER);
    for (int i = 0; i < MAX_EMAILS_NUM; ++i)
    {
        SetWindowPos(GetDlgItem(hwnd, 1010 + 10 * i + 1), NULL, 80, 290 + 30 * i, 75, 20, SWP_NOZORDER);
        SetWindowPos(GetDlgItem(hwnd, 1010 + 10 * i + 2), NULL, 160, 290 + 30 * i, (w - 20 - 20) / 2 - 160, 20, SWP_NOZORDER);
        SetWindowPos(GetDlgItem(hwnd, 1010 + 10 * i + 3), NULL, w/2, 290 + 30 * i, 155, 20, SWP_NOZORDER);
        SetWindowPos(GetDlgItem(hwnd, 1010 + 10 * i + 4), NULL, w/2 + 160, 290 + 30 * i, (w - 20 - 20) / 2 - 160, 20, SWP_NOZORDER);
    }

    SetWindowPos(GetDlgItem(hwnd, 12), NULL, 20, EMAILS_FOOT_Y, 160, 20, SWP_NOZORDER);
    SetWindowPos(hYourLink, NULL, 160, EMAILS_FOOT_Y, w - 160 - 15, 20, SWP_NOZORDER);

    SetWindowPos(hCheckboxDelete, NULL, 160, EMAILS_FOOT_Y + 40, w - 160 - 15, 20, SWP_NOZORDER);

    SetWindowPos(hCreateButton, NULL, (w-200)/2, EMAILS_FOOT_Y + 80, 200, 30, SWP_NOZORDER);

    SetWindowPos(hProgressBar, NULL, 20, EMAILS_FOOT_Y + 120, w - 20*2, 20, SWP_NOZORDER);
    SetWindowPos(GetDlgItem(hwnd, 2), NULL, 20, EMAILS_FOOT_Y + 160, w - 20 * 2, h > (EMAILS_FOOT_Y+260) ? (h - EMAILS_FOOT_Y - 180) : 80, SWP_NOZORDER);
}

void DetectEmails(HWND hwnd)
{
    char phpDir[MY_MAX_PATH], templateDir[MY_MAX_PATH];
    GetWindowText(hInputPHPDir, phpDir, MY_MAX_PATH);
    GetWindowText(hInputTemplateDir, templateDir, MY_MAX_PATH);

    int iEmail = 0;
    for (const auto& entry : fs::directory_iterator(std::string(phpDir)))
    {
        std::string newFile = entry.path().filename().string();
        std::string templateFilePath = std::string(templateDir) + "\\" + newFile;
        if (entry.is_directory())
            continue;
        if (newFile.find("broadcast") == std::string::npos)
            continue;
        SetDlgItemText(hwnd, 1010 + 10 * iEmail + 2, newFile.c_str());
        SetDlgItemText(hwnd, 1010 + 10 * iEmail + 4, (std::string("broadcast") + std::to_string(iEmail+1) + ".txt").c_str());
        iEmail++;
    }
}