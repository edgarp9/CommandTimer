#include <windows.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <format>
#include <stdexcept>
#include <algorithm>
#include <string_view>
#include <optional>
#include <limits>


//================================================================================================//
// Global Variables and Constants
//================================================================================================//

// --- Application State ---
enum class TimerState
{
    STOPPED,
    RUNNING,
    PAUSED
};

// --- Control IDs ---
constexpr int IDC_EDIT_HOUR = 101;
constexpr int IDC_EDIT_MIN = 102;
constexpr int IDC_EDIT_SEC = 103;
constexpr int IDC_COMBO_CMD = 104;
constexpr int IDC_BTN_START = 105;
constexpr int IDC_BTN_PAUSE = 106;
constexpr int IDC_BTN_RESET = 107;
constexpr int IDC_STATIC_TIMER_DISPLAY = 108;
constexpr int IDC_BTN_HOMEPAGE = 109;
constexpr int IDC_BTN_PRESET1 = 110;
constexpr int IDC_BTN_PRESET2 = 111;
constexpr int IDC_BTN_PRESET3 = 112;
constexpr int MAX_HISTORY = 20;

// --- CommandLine Options ---
struct CommandLineOptions {
    bool startImmediately = false;
    int hours = 0;
    int minutes = 0;
    int seconds = 0;
    std::wstring command = std::wstring();
};

// --- Global Handles and Variables ---
HINSTANCE g_hInst;
HWND      g_hWnd;
UINT_PTR  g_timerId = 0;
int       g_remainingSeconds = 0;
TimerState g_timerState = TimerState::STOPPED;
HFONT     g_hDefaultFont = NULL;
HFONT     g_hTimerFont = NULL;
wchar_t   g_iniFilePath[MAX_PATH];
int       g_presetMinutes1 = 5;
int       g_presetMinutes2 = 30;
int       g_presetMinutes3 = 50;


//================================================================================================//
// Function Prototypes
//================================================================================================//

// --- Core Win32 Functions ---
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int);
std::optional<CommandLineOptions> ParseCommandLineArgs();

// --- UI Management ---
void CreateMainWindowControls(HWND hWnd);
void UpdateTimerDisplay(HWND hWnd);
void UpdateControlStatesByTimerStatus(HWND hWnd);

// --- Event Handlers ---
void OnStartButtonClick(HWND hWnd);
void OnPauseButtonClick(HWND hWnd);
void OnResetButtonClick(HWND hWnd);
void OnHomepageButtonClick(HWND hWnd);
void OnPresetButtonClick(HWND hWnd, int presetMinutes);

// --- Core Logic ---
void ExecuteTimerCommand(HWND hWnd);

// --- INI File and History Management ---
void SetIniFilePath();
void LoadPresetTimes();
void LoadCommandHistory(HWND hWnd);
void SaveCommandHistory(HWND hWnd);

// --- Utility ---
std::optional<int> ValidateAndParsePositiveInt(std::wstring_view s);


//================================================================================================//
// Core Win32 Functions
//================================================================================================//

/**
 * @brief The main entry point for the application.
 */
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    g_hInst = hInstance;
    SetIniFilePath();
    LoadPresetTimes();

    const wchar_t CLASS_NAME[] = L"CommandTimerClass";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    g_hWnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Command Timer v1.3",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 420, 320,
        NULL, NULL, hInstance, NULL
    );

    if (g_hWnd == NULL)
    {
        return 0;
    }

    // Command line parsing logic remains the same...
    bool startImmediately = false;
    if (auto retCmdOptions = ParseCommandLineArgs(); retCmdOptions.has_value()) {
        const auto& cmdOptions = retCmdOptions.value();

        startImmediately = cmdOptions.startImmediately;

        g_remainingSeconds = (cmdOptions.hours * 3600) + (cmdOptions.minutes * 60) + cmdOptions.seconds;
        if (g_remainingSeconds > 0) {
            SetDlgItemInt(g_hWnd, IDC_EDIT_HOUR, cmdOptions.hours, FALSE);
            SetDlgItemInt(g_hWnd, IDC_EDIT_MIN, cmdOptions.minutes, FALSE);
            SetDlgItemInt(g_hWnd, IDC_EDIT_SEC, cmdOptions.seconds, FALSE);
            UpdateTimerDisplay(g_hWnd);
        }

        if (!cmdOptions.command.empty()) {
            SetDlgItemText(g_hWnd, IDC_COMBO_CMD, cmdOptions.command.c_str());
        }
    }
    else
    {
        const wchar_t* messageText = L"Invalid Argument Error: Check your arguments.\n"
            L"Supports the arguments -start -h -m -s -cmd.\n"
            L"-cmd must be the last argument.\n"
            L"Example: CommandTimer.exe -start -m 30 -cmd \"notepad.exe\"";
        MessageBoxW(NULL, messageText, L"Argument Error", MB_OK | MB_ICONERROR);
    }


    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);

    if (startImmediately && g_remainingSeconds > 0)
    {
        PostMessage(g_hWnd, WM_COMMAND, MAKEWPARAM(IDC_BTN_START, BN_CLICKED), 0);
    }

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

/**
 * @brief Processes messages for the main window.
 */
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        CreateMainWindowControls(hWnd);
        break;
    }
    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDC_BTN_START:    OnStartButtonClick(hWnd);    break;
        case IDC_BTN_PAUSE:    OnPauseButtonClick(hWnd);    break;
        case IDC_BTN_RESET:    OnResetButtonClick(hWnd);    break;
        case IDC_BTN_HOMEPAGE: OnHomepageButtonClick(hWnd); break;
        case IDC_BTN_PRESET1:  OnPresetButtonClick(hWnd, g_presetMinutes1); break;
        case IDC_BTN_PRESET2:  OnPresetButtonClick(hWnd, g_presetMinutes2); break;
        case IDC_BTN_PRESET3:  OnPresetButtonClick(hWnd, g_presetMinutes3); break;
        }
        break;
    }
    case WM_TIMER:
    {
        if (g_remainingSeconds > 0)
        {
            g_remainingSeconds--;
            UpdateTimerDisplay(hWnd);
        }

        if (g_remainingSeconds <= 0)
        {
            KillTimer(hWnd, g_timerId);
            g_timerId = 0;
            g_timerState = TimerState::STOPPED;
            ExecuteTimerCommand(hWnd);
            UpdateControlStatesByTimerStatus(hWnd);
        }
        break;
    }
    case WM_DESTROY:
    {
        SaveCommandHistory(hWnd);
        if (g_timerId != 0)
        {
            KillTimer(hWnd, g_timerId);
        }
        if (g_hDefaultFont) DeleteObject(g_hDefaultFont);
        if (g_hTimerFont) DeleteObject(g_hTimerFont);
        PostQuitMessage(0);
        break;
    }
    default:
    {
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    }
    return 0;
}

std::optional<CommandLineOptions> ParseCommandLineArgs() {
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    if (argv == NULL || argc < 2) {
        if (argv) LocalFree(argv);
        return CommandLineOptions{};
    }

    CommandLineOptions options;
    bool success = true;

    for (int i = 1; i < argc; ++i) {
        std::wstring_view arg = argv[i];

        if (arg == L"-start") {
            options.startImmediately = true;
        }
        else if (arg == L"-h" || arg == L"-m" || arg == L"-s") {
            if (i + 1 >= argc) {
                success = false;
                break;
            }

            auto value = ValidateAndParsePositiveInt(argv[++i]);
            if (!value.has_value()) {
                success = false;
                break;
            }

            if (arg == L"-h") options.hours = value.value();
            else if (arg == L"-m") options.minutes = value.value();
            else if (arg == L"-s") options.seconds = value.value();
        }
        else if (arg == L"-cmd") {
            if (i + 1 >= argc) {
                success = false;
                break;
            }

            options.command = argv[++i];
            while (i + 1 < argc && argv[i + 1][0] != L'-') {
                options.command += L" ";
                options.command += argv[++i];
            }
        }
        else {
            success = false;
            break;
        }
    }

    LocalFree(argv);

    if (success) {
        return options;
    }

    return std::nullopt;
}

//================================================================================================//
// UI Management Functions
//================================================================================================//

/**
 * @brief Creates and positions all UI controls in the main window.
 */
void CreateMainWindowControls(HWND hWnd)
{
    // --- Create Fonts ---
    g_hDefaultFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    g_hTimerFont = CreateFont(50, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");

    // --- Create Controls ---
    // Time setting controls
    CreateWindow(L"static", L"Set Time:", WS_CHILD | WS_VISIBLE, 20, 20, 80, 20, hWnd, NULL, g_hInst, NULL);
    CreateWindow(L"edit", L"0", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | ES_CENTER, 110, 20, 50, 25, hWnd, (HMENU)(INT_PTR)IDC_EDIT_HOUR, g_hInst, NULL);
    CreateWindow(L"static", L"h", WS_CHILD | WS_VISIBLE, 165, 22, 20, 20, hWnd, NULL, g_hInst, NULL);
    CreateWindow(L"edit", L"0", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | ES_CENTER, 195, 20, 50, 25, hWnd, (HMENU)(INT_PTR)IDC_EDIT_MIN, g_hInst, NULL);
    CreateWindow(L"static", L"m", WS_CHILD | WS_VISIBLE, 250, 22, 20, 20, hWnd, NULL, g_hInst, NULL);
    CreateWindow(L"edit", L"0", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | ES_CENTER, 280, 20, 50, 25, hWnd, (HMENU)(INT_PTR)IDC_EDIT_SEC, g_hInst, NULL);
    CreateWindow(L"static", L"s", WS_CHILD | WS_VISIBLE, 335, 22, 20, 20, hWnd, NULL, g_hInst, NULL);

    // Command execution controls
    CreateWindow(L"static", L"Command:", WS_CHILD | WS_VISIBLE, 20, 60, 80, 20, hWnd, NULL, g_hInst, NULL);
    CreateWindow(L"combobox", L"", CBS_DROPDOWN | WS_CHILD | WS_VISIBLE | WS_VSCROLL, 110, 60, 280, 150, hWnd, (HMENU)(INT_PTR)IDC_COMBO_CMD, g_hInst, NULL);

    // Action buttons
    CreateWindow(L"button", L"Start", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 100, 95, 30, hWnd, (HMENU)(INT_PTR)IDC_BTN_START, g_hInst, NULL);
    CreateWindow(L"button", L"Pause", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 120, 100, 85, 30, hWnd, (HMENU)(INT_PTR)IDC_BTN_PAUSE, g_hInst, NULL);
    CreateWindow(L"button", L"Reset", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 210, 100, 85, 30, hWnd, (HMENU)(INT_PTR)IDC_BTN_RESET, g_hInst, NULL);
    CreateWindow(L"button", L"Homepage", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 300, 100, 90, 30, hWnd, (HMENU)(INT_PTR)IDC_BTN_HOMEPAGE, g_hInst, NULL);

    // --- Preset Buttons ---
    std::wstring presetLabel1 = std::format(L"{} Min", g_presetMinutes1);
    std::wstring presetLabel2 = std::format(L"{} Min", g_presetMinutes2);
    std::wstring presetLabel3 = std::format(L"{} Min", g_presetMinutes3);
    CreateWindow(L"button", presetLabel1.c_str(), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 140, 120, 30, hWnd, (HMENU)(INT_PTR)IDC_BTN_PRESET1, g_hInst, NULL);
    CreateWindow(L"button", presetLabel2.c_str(), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 145, 140, 120, 30, hWnd, (HMENU)(INT_PTR)IDC_BTN_PRESET2, g_hInst, NULL);
    CreateWindow(L"button", presetLabel3.c_str(), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 270, 140, 120, 30, hWnd, (HMENU)(INT_PTR)IDC_BTN_PRESET3, g_hInst, NULL);


    // Timer display (position adjusted)
    HWND hStaticTimerDisplay = CreateWindow(L"static", L"00:00:00", WS_CHILD | WS_VISIBLE | SS_CENTER, 20, 185, 370, 50, hWnd, (HMENU)(INT_PTR)IDC_STATIC_TIMER_DISPLAY, g_hInst, NULL);

    // --- Apply Fonts ---
    EnumChildWindows(hWnd, [](HWND hwnd, LPARAM lParam) -> BOOL {
        SendMessage(hwnd, WM_SETFONT, (WPARAM)lParam, TRUE);
        return TRUE;
        }, (LPARAM)g_hDefaultFont);

    SendMessage(hStaticTimerDisplay, WM_SETFONT, (WPARAM)g_hTimerFont, TRUE);

    // --- Load Initial Data ---
    LoadCommandHistory(hWnd);
    UpdateControlStatesByTimerStatus(hWnd);
}

/**
 * @brief Updates the timer display with the current remaining time.
 */
void UpdateTimerDisplay(HWND hWnd)
{
    int h = g_remainingSeconds / 3600;
    int m = (g_remainingSeconds % 3600) / 60;
    int s = g_remainingSeconds % 60;
    std::wstring timeString = std::format(L"{:02}:{:02}:{:02}", h, m, s);
    SetDlgItemText(hWnd, IDC_STATIC_TIMER_DISPLAY, timeString.c_str());
}

/**
 * @brief Enables or disables UI controls based on the current timer state.
 */
void UpdateControlStatesByTimerStatus(HWND hWnd)
{
    bool isStopped = (g_timerState == TimerState::STOPPED);
    bool isRunning = (g_timerState == TimerState::RUNNING);
    bool isPaused = (g_timerState == TimerState::PAUSED);

    // Enable time and command inputs only when stopped
    EnableWindow(GetDlgItem(hWnd, IDC_EDIT_HOUR), isStopped);
    EnableWindow(GetDlgItem(hWnd, IDC_EDIT_MIN), isStopped);
    EnableWindow(GetDlgItem(hWnd, IDC_EDIT_SEC), isStopped);
    EnableWindow(GetDlgItem(hWnd, IDC_COMBO_CMD), isStopped);

    // Also enable preset buttons only when stopped
    EnableWindow(GetDlgItem(hWnd, IDC_BTN_PRESET1), isStopped);
    EnableWindow(GetDlgItem(hWnd, IDC_BTN_PRESET2), isStopped);
    EnableWindow(GetDlgItem(hWnd, IDC_BTN_PRESET3), isStopped);


    // Update button states
    EnableWindow(GetDlgItem(hWnd, IDC_BTN_START), isStopped || isPaused);
    EnableWindow(GetDlgItem(hWnd, IDC_BTN_PAUSE), isRunning);
    EnableWindow(GetDlgItem(hWnd, IDC_BTN_RESET), isRunning || isPaused);

    // Change "Start" button text to "Resume" if paused
    SetDlgItemText(hWnd, IDC_BTN_START, isPaused ? L"Resume" : L"Start");
}

//================================================================================================//
// Event Handlers
//================================================================================================//

/**
 * @brief Handles the 'Start/Resume' button click event.
 */
void OnStartButtonClick(HWND hWnd)
{
    if (g_timerState == TimerState::STOPPED)
    {
        wchar_t hourStr[10], minStr[10], secStr[10];
        GetDlgItemText(hWnd, IDC_EDIT_HOUR, hourStr, 10);
        GetDlgItemText(hWnd, IDC_EDIT_MIN, minStr, 10);
        GetDlgItemText(hWnd, IDC_EDIT_SEC, secStr, 10);

        auto v = ValidateAndParsePositiveInt(hourStr);
        int hours = (v.has_value()) ? v.value() : 0;
        v = ValidateAndParsePositiveInt(minStr);
        int minutes = (v.has_value()) ? v.value() : 0;
        v = ValidateAndParsePositiveInt(secStr);
        int seconds = (v.has_value()) ? v.value() : 0;

        g_remainingSeconds = (hours * 3600) + (minutes * 60) + seconds;
    }

    if (g_remainingSeconds > 0)
    {
        g_timerId = SetTimer(hWnd, 1, 1000, NULL);
        g_timerState = TimerState::RUNNING;
        SaveCommandHistory(hWnd);
    }
    else if (g_timerState == TimerState::STOPPED)
    {
        MessageBox(hWnd, L"Please enter a time greater than 0 seconds.", L"Input Error", MB_OK | MB_ICONWARNING);
    }

    UpdateControlStatesByTimerStatus(hWnd);
}

/**
 * @brief Handles the 'Pause' button click event.
 */
void OnPauseButtonClick(HWND hWnd)
{
    if (g_timerState == TimerState::RUNNING)
    {
        KillTimer(hWnd, g_timerId);
        g_timerId = 0;
        g_timerState = TimerState::PAUSED;
        UpdateControlStatesByTimerStatus(hWnd);
    }
}

/**
 * @brief Handles the 'Reset' button click event.
 */
void OnResetButtonClick(HWND hWnd)
{
    if (g_timerId != 0)
    {
        KillTimer(hWnd, g_timerId);
        g_timerId = 0;
    }
    g_remainingSeconds = 0;
    g_timerState = TimerState::STOPPED;
    UpdateTimerDisplay(hWnd);
    UpdateControlStatesByTimerStatus(hWnd);
}

/**
 * @brief Handles the 'Homepage' button click event.
 */
void OnHomepageButtonClick(HWND hWnd)
{
    ShellExecute(hWnd, L"open", L"https://github.com/edgarp9/CommandTimer", NULL, NULL, SW_SHOWNORMAL);
}

/**
 * @brief Handles a click on one of the new preset time buttons.
 */
void OnPresetButtonClick(HWND hWnd, int presetMinutes)
{
    if (g_timerState != TimerState::STOPPED) return;

    g_remainingSeconds = presetMinutes * 60;

    if (g_remainingSeconds > 0)
    {
        // Update the edit controls to reflect the preset time
        int h = g_remainingSeconds / 3600;
        int m = (g_remainingSeconds % 3600) / 60;
        int s = g_remainingSeconds % 60;
        SetDlgItemInt(hWnd, IDC_EDIT_HOUR, h, FALSE);
        SetDlgItemInt(hWnd, IDC_EDIT_MIN, m, FALSE);
        SetDlgItemInt(hWnd, IDC_EDIT_SEC, s, FALSE);
        UpdateTimerDisplay(hWnd);

        // Start the timer
        g_timerId = SetTimer(hWnd, 1, 1000, NULL);
        g_timerState = TimerState::RUNNING;
        SaveCommandHistory(hWnd);
        UpdateControlStatesByTimerStatus(hWnd);
    }
}


//================================================================================================//
// Core Logic Functions
//================================================================================================//

/**
 * @brief Executes the command specified in the combo box when the timer finishes.
 */
void ExecuteTimerCommand(HWND hWnd)
{
    wchar_t cmd[512]{};
    GetDlgItemTextW(hWnd, IDC_COMBO_CMD, cmd, 512);
    if (cmd[0] == L'\0') return;

    SaveCommandHistory(hWnd); // Ensure the executed command is saved

    // Try executing with ShellExecute first
    HINSTANCE hInst = ShellExecute(hWnd, L"open", cmd, NULL, NULL, SW_SHOWNORMAL);
    if ((INT_PTR)hInst <= 32)
    {
        // If ShellExecute fails, fallback to CreateProcess
        STARTUPINFOW si{ sizeof(si) };
        PROCESS_INFORMATION pi{};
        std::vector<wchar_t> line(cmd, cmd + wcslen(cmd) + 1);

        if (!CreateProcessW(NULL, line.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
        {
            DWORD err = GetLastError();
            std::wstring errorMsg = std::format(L"Failed to execute command (Error code: {})", err);
            MessageBoxW(hWnd, errorMsg.c_str(), L"Execution Error", MB_OK | MB_ICONERROR);
            return;
        }
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

//================================================================================================//
// INI File and History Management
//================================================================================================//

/**
 * @brief Determines the path for the INI file (same directory as the exe).
 */
void SetIniFilePath()
{
    GetModuleFileNameW(NULL, g_iniFilePath, MAX_PATH);
    wchar_t* dot = wcsrchr(g_iniFilePath, L'.');
    if (dot)
    {
        wcscpy_s(dot, MAX_PATH - (dot - g_iniFilePath), L".ini");
    }
    else
    {
        wcscat_s(g_iniFilePath, MAX_PATH, L".ini");
    }
}

/**
 * @brief Loads preset times from the INI file. Uses defaults if not found.
 */
void LoadPresetTimes()
{
    const wchar_t* section = L"PresetTimes";
    g_presetMinutes1 = GetPrivateProfileIntW(section, L"Time1", 5, g_iniFilePath);
    g_presetMinutes2 = GetPrivateProfileIntW(section, L"Time2", 30, g_iniFilePath);
    g_presetMinutes3 = GetPrivateProfileIntW(section, L"Time3", 50, g_iniFilePath);

    // Write the values back to the INI file if it doesn't exist, to make them discoverable
    WritePrivateProfileStringW(section, L"Time1", std::to_wstring(g_presetMinutes1).c_str(), g_iniFilePath);
    WritePrivateProfileStringW(section, L"Time2", std::to_wstring(g_presetMinutes2).c_str(), g_iniFilePath);
    WritePrivateProfileStringW(section, L"Time3", std::to_wstring(g_presetMinutes3).c_str(), g_iniFilePath);
}


/**
 * @brief Loads the command history from the INI file into the ComboBox.
 */
void LoadCommandHistory(HWND hWnd)
{
    HWND hCombo = GetDlgItem(hWnd, IDC_COMBO_CMD);
    const wchar_t* section = L"CommandHistory";

    int count = GetPrivateProfileIntW(section, L"Count", 0, g_iniFilePath);

    for (int i = 1; i <= count; ++i)
    {
        wchar_t key[20];
        swprintf_s(key, L"Command%d", i);
        wchar_t cmd[512];
        GetPrivateProfileStringW(section, key, L"", cmd, 512, g_iniFilePath);
        if (wcslen(cmd) > 0)
        {
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)cmd);
        }
    }

    if (count > 0)
    {
        SendMessage(hCombo, CB_SETCURSEL, 0, 0);
    }
    else
    {
        // Provide a default command if history is empty
        SetDlgItemTextW(hWnd, IDC_COMBO_CMD, L"notepad.exe");
    }
}

/**
 * @brief Saves the current command history from the ComboBox to the INI file.
 * It avoids duplicates and limits the history size.
 */
void SaveCommandHistory(HWND hWnd)
{
    const wchar_t* section = L"CommandHistory";
    HWND hCombo = GetDlgItem(hWnd, IDC_COMBO_CMD);

    std::vector<std::wstring> history;
    wchar_t currentCmd[512];
    GetDlgItemTextW(hWnd, IDC_COMBO_CMD, currentCmd, 512);

    // 1. Add the current command to the top of the history if it's not empty.
    if (wcslen(currentCmd) > 0)
    {
        history.push_back(currentCmd);
    }

    // 2. Read existing commands from the INI file.
    int oldCount = GetPrivateProfileIntW(section, L"Count", 0, g_iniFilePath);
    for (int i = 1; i <= oldCount; ++i)
    {
        if (history.size() >= MAX_HISTORY) break;

        wchar_t key[20];
        swprintf_s(key, L"Command%d", i);
        wchar_t oldCmd[512];
        GetPrivateProfileStringW(section, key, L"", oldCmd, 512, g_iniFilePath);

        // 3. Add old command only if it's not already in our new history list.
        if (wcslen(oldCmd) > 0 &&
            std::find(history.begin(), history.end(), oldCmd) == history.end())
        {
            history.push_back(oldCmd);
        }
    }

    // 4. Clear the old section and write the new, cleaned history to the INI file.
    WritePrivateProfileStringW(section, NULL, NULL, g_iniFilePath);
    WritePrivateProfileStringW(section, L"Count", std::to_wstring(history.size()).c_str(), g_iniFilePath);

    for (size_t i = 0; i < history.size(); ++i)
    {
        wchar_t key[20];
        swprintf_s(key, L"Command%zu", i + 1);
        WritePrivateProfileStringW(section, key, history[i].c_str(), g_iniFilePath);
    }

    // 5. Update the ComboBox UI to match the newly saved history.
    SendMessage(hCombo, CB_RESETCONTENT, 0, 0);
    for (const auto& cmd : history)
    {
        SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)cmd.c_str());
    }

    // Restore the text that the user might have been editing.
    SetDlgItemTextW(hWnd, IDC_COMBO_CMD, currentCmd);
}

//================================================================================================//
// Utility
//================================================================================================//

std::optional<int> ValidateAndParsePositiveInt(std::wstring_view s) {
    if (s.empty()) return std::nullopt;

    int value = 0;
    constexpr int INT_MAX_VAL = (std::numeric_limits<int>::max)();

    for (wchar_t ch : s) {
        if (ch < L'0' || ch > L'9') return std::nullopt;
        int digit = ch - L'0';
        if (value > (INT_MAX_VAL - digit) / 10) return std::nullopt;
        value = value * 10 + digit;
    }
    return value;
}