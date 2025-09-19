#include <windows.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <format>
#include <stdexcept>

// Enum to represent the state of the timer.
enum class TimerState {
    STOPPED,
    RUNNING,
    PAUSED
};

// --- Global Variables ---
HINSTANCE g_hInst;
HWND g_hWnd;
UINT_PTR g_timerId = 0;
int g_remainingSeconds = 0;
TimerState g_timerState = TimerState::STOPPED;
HFONT g_hDefaultFont = NULL; // Default font for UI controls.
HFONT g_hTimerFont = NULL; // Large font for the timer display.

// --- Control IDs ---
#define IDC_EDIT_HOUR 101
#define IDC_EDIT_MIN 102
#define IDC_EDIT_SEC 103
#define IDC_EDIT_CMD 104
#define IDC_BTN_START 105
#define IDC_BTN_PAUSE 106
#define IDC_BTN_RESET 107
#define IDC_STATIC_TIMER_DISPLAY 108
#define IDC_BTN_HOMEPAGE 109

// --- Function Prototypes ---
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void InitializeWindowControls(HWND);
void RefreshTimerDisplay(HWND);
void RunTimerCompletionCommand(HWND);
void UpdateUIControlStates(HWND);

// Button Command Handlers
void OnStartButtonClick(HWND);
void OnPauseButtonClick(HWND);
void OnResetButtonClick(HWND);
void OnHomepageButtonClick(HWND);


/**
 * @brief The main entry point for the application.
 */
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    g_hInst = hInstance;

    const wchar_t CLASS_NAME[] = L"CommandTimerClass";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    g_hWnd = CreateWindowEx(
        0, CLASS_NAME, L"Command Timer v1",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 420, 280,
        NULL, NULL, hInstance, NULL);

    if (g_hWnd == NULL) {
        return 0;
    }

    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

/**
 * @brief Processes messages for the main window.
 */
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        InitializeWindowControls(hWnd);
        break;

    case WM_COMMAND:
    {
        // Delegate command handling to specific functions.
        switch (LOWORD(wParam)) {
        case IDC_BTN_START:
            OnStartButtonClick(hWnd);
            break;
        case IDC_BTN_PAUSE:
            OnPauseButtonClick(hWnd);
            break;
        case IDC_BTN_RESET:
            OnResetButtonClick(hWnd);
            break;
        case IDC_BTN_HOMEPAGE:
            OnHomepageButtonClick(hWnd);
            break;
        }
    }
    break;

    case WM_TIMER:
    {
        if (g_remainingSeconds > 0) {
            g_remainingSeconds--;
            RefreshTimerDisplay(hWnd);
        }

        if (g_remainingSeconds <= 0) {
            KillTimer(hWnd, g_timerId);
            g_timerId = 0;
            g_timerState = TimerState::STOPPED;
            RunTimerCompletionCommand(hWnd);
            UpdateUIControlStates(hWnd);
            MessageBox(hWnd, L"Timer finished and command executed.", L"Notification", MB_OK | MB_ICONINFORMATION);
        }
    }
    break;

    case WM_DESTROY:
        if (g_timerId != 0) {
            KillTimer(hWnd, g_timerId);
        }
        // Clean up GDI font resources.
        if (g_hDefaultFont) DeleteObject(g_hDefaultFont);
        if (g_hTimerFont) DeleteObject(g_hTimerFont);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

/**
 * @brief Handles the 'Start/Resume' button click event.
 */
void OnStartButtonClick(HWND hWnd) {
    if (g_timerState == TimerState::STOPPED || g_timerState == TimerState::PAUSED) {
        // If the timer is stopped, read the input values.
        if (g_timerState == TimerState::STOPPED) {
            wchar_t hourStr[10], minStr[10], secStr[10];
            GetDlgItemText(hWnd, IDC_EDIT_HOUR, hourStr, 10);
            GetDlgItemText(hWnd, IDC_EDIT_MIN, minStr, 10);
            GetDlgItemText(hWnd, IDC_EDIT_SEC, secStr, 10);

            try {
                int hours = (wcslen(hourStr) > 0) ? std::stoi(hourStr) : 0;
                int minutes = (wcslen(minStr) > 0) ? std::stoi(minStr) : 0;
                int seconds = (wcslen(secStr) > 0) ? std::stoi(secStr) : 0;
                g_remainingSeconds = (hours * 3600) + (minutes * 60) + seconds;
            }
            catch (const std::exception&) {
                MessageBox(hWnd, L"Please enter valid numbers for the time.", L"Input Error", MB_OK | MB_ICONERROR);
                g_remainingSeconds = 0;
                return;
            }
        }

        // Start the timer if there is time remaining.
        if (g_remainingSeconds > 0) {
            g_timerId = SetTimer(hWnd, 1, 1000, NULL);
            g_timerState = TimerState::RUNNING;
            UpdateUIControlStates(hWnd);
        }
        else if (g_timerState == TimerState::STOPPED) {
            MessageBox(hWnd, L"Please enter a time greater than 0 seconds.", L"Input Error", MB_OK | MB_ICONWARNING);
        }
    }
}

/**
 * @brief Handles the 'Pause' button click event.
 */
void OnPauseButtonClick(HWND hWnd) {
    if (g_timerState == TimerState::RUNNING) {
        KillTimer(hWnd, g_timerId);
        g_timerId = 0;
        g_timerState = TimerState::PAUSED;
        UpdateUIControlStates(hWnd);
    }
}

/**
 * @brief Handles the 'Reset' button click event.
 */
void OnResetButtonClick(HWND hWnd) {
    if (g_timerId != 0) {
        KillTimer(hWnd, g_timerId);
        g_timerId = 0;
    }
    g_remainingSeconds = 0;
    g_timerState = TimerState::STOPPED;
    // Reset the display, but keep the user's input values.
    RefreshTimerDisplay(hWnd);
    UpdateUIControlStates(hWnd);
}

/**
 * @brief Handles the 'Homepage' button click event.
 */
void OnHomepageButtonClick(HWND hWnd) {
    ShellExecute(hWnd, L"open", L"https://github.com/edgarp9/CommandTimer", NULL, NULL, SW_SHOWNORMAL);
}

/**
 * @brief Creates and positions all the UI controls in the main window.
 * @param hWnd Handle to the main window.
 */
void InitializeWindowControls(HWND hWnd) {
    // Create fonts for the UI.
    g_hDefaultFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    g_hTimerFont = CreateFont(50, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");

    // Time setting controls.
    CreateWindow(L"static", L"Set Time:", WS_CHILD | WS_VISIBLE, 20, 20, 80, 20, hWnd, NULL, g_hInst, NULL);
    CreateWindow(L"edit", L"0", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | ES_CENTER, 110, 20, 50, 25, hWnd, (HMENU)IDC_EDIT_HOUR, g_hInst, NULL);
    CreateWindow(L"static", L"h", WS_CHILD | WS_VISIBLE, 165, 22, 20, 20, hWnd, NULL, g_hInst, NULL);
    CreateWindow(L"edit", L"0", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | ES_CENTER, 195, 20, 50, 25, hWnd, (HMENU)IDC_EDIT_MIN, g_hInst, NULL);
    CreateWindow(L"static", L"m", WS_CHILD | WS_VISIBLE, 250, 22, 20, 20, hWnd, NULL, g_hInst, NULL);
    CreateWindow(L"edit", L"0", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | ES_CENTER, 280, 20, 50, 25, hWnd, (HMENU)IDC_EDIT_SEC, g_hInst, NULL);
    CreateWindow(L"static", L"s", WS_CHILD | WS_VISIBLE, 335, 22, 20, 20, hWnd, NULL, g_hInst, NULL);

    // Command execution controls.
    CreateWindow(L"static", L"Command:", WS_CHILD | WS_VISIBLE, 20, 60, 80, 20, hWnd, NULL, g_hInst, NULL);
    CreateWindow(L"edit", L"notepad.exe", WS_CHILD | WS_VISIBLE | WS_BORDER, 110, 60, 280, 25, hWnd, (HMENU)IDC_EDIT_CMD, g_hInst, NULL);

    // Action buttons.
    CreateWindow(L"button", L"Start", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 100, 95, 30, hWnd, (HMENU)IDC_BTN_START, g_hInst, NULL);
    CreateWindow(L"button", L"Pause", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 120, 100, 85, 30, hWnd, (HMENU)IDC_BTN_PAUSE, g_hInst, NULL);
    CreateWindow(L"button", L"Reset", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 210, 100, 85, 30, hWnd, (HMENU)IDC_BTN_RESET, g_hInst, NULL);
    CreateWindow(L"button", L"Homepage", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 300, 100, 90, 30, hWnd, (HMENU)IDC_BTN_HOMEPAGE, g_hInst, NULL);

    // Timer display static text.
    HWND hStaticTimerDisplay = CreateWindow(L"static", L"00:00:00", WS_CHILD | WS_VISIBLE | SS_CENTER, 20, 150, 370, 50, hWnd, (HMENU)IDC_STATIC_TIMER_DISPLAY, g_hInst, NULL);

    // Apply the default font to all child windows (controls).
    EnumChildWindows(hWnd, [](HWND hwnd, LPARAM lParam) -> BOOL {
        SendMessage(hwnd, WM_SETFONT, (WPARAM)lParam, TRUE);
        return TRUE;
        }, (LPARAM)g_hDefaultFont);

    // Apply the special larger font to the timer display.
    SendMessage(hStaticTimerDisplay, WM_SETFONT, (WPARAM)g_hTimerFont, TRUE);

    UpdateUIControlStates(hWnd);
}

/**
 * @brief Updates the timer display with the current remaining time.
 * @param hWnd Handle to the main window.
 */
void RefreshTimerDisplay(HWND hWnd) {
    int h = g_remainingSeconds / 3600;
    int m = (g_remainingSeconds % 3600) / 60;
    int s = g_remainingSeconds % 60;
    std::wstring timeString = std::format(L"{:02}:{:02}:{:02}", h, m, s);
    SetDlgItemText(hWnd, IDC_STATIC_TIMER_DISPLAY, timeString.c_str());
}

/**
 * @brief Executes the command specified in the command input box.
 * @param hWnd Handle to the main window.
 */
void RunTimerCompletionCommand(HWND hWnd) {
    wchar_t cmd[512]{};
    GetDlgItemTextW(hWnd, IDC_EDIT_CMD, cmd, 512);
    if (cmd[0] == L'\0') return;

    // Use ShellExecute for broader compatibility (URLs, files, folders).
    HINSTANCE hInst = ShellExecute(hWnd, L"open", cmd, NULL, NULL, SW_SHOWNORMAL);
    if ((INT_PTR)hInst <= 32) {
        // If ShellExecute fails, fall back to CreateProcess (for commands with arguments).
        STARTUPINFOW si{ sizeof(si) };
        PROCESS_INFORMATION pi{};
        std::vector<wchar_t> line(cmd, cmd + wcslen(cmd) + 1);
        if (!CreateProcessW(NULL, line.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            DWORD err = GetLastError();
            wchar_t buf[256];
            swprintf(buf, 256, L"Failed to execute command (Error code: %lu)", err);
            MessageBoxW(hWnd, buf, L"Execution Error", MB_OK | MB_ICONERROR);
            return;
        }
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

/**
 * @brief Enables or disables UI controls based on the current timer state.
 * @param hWnd Handle to the main window.
 */
void UpdateUIControlStates(HWND hWnd) {
    bool isStopped = (g_timerState == TimerState::STOPPED);
    bool isRunning = (g_timerState == TimerState::RUNNING);
    bool isPaused = (g_timerState == TimerState::PAUSED);

    // Enable input fields only when the timer is stopped.
    EnableWindow(GetDlgItem(hWnd, IDC_EDIT_HOUR), isStopped);
    EnableWindow(GetDlgItem(hWnd, IDC_EDIT_MIN), isStopped);
    EnableWindow(GetDlgItem(hWnd, IDC_EDIT_SEC), isStopped);
    EnableWindow(GetDlgItem(hWnd, IDC_EDIT_CMD), isStopped);

    // Update button states.
    EnableWindow(GetDlgItem(hWnd, IDC_BTN_START), isStopped || isPaused);
    EnableWindow(GetDlgItem(hWnd, IDC_BTN_PAUSE), isRunning);
    EnableWindow(GetDlgItem(hWnd, IDC_BTN_RESET), isRunning || isPaused);

    // Change "Start" button text to "Resume" when paused.
    if (isPaused) {
        SetDlgItemText(hWnd, IDC_BTN_START, L"Resume");
    }
    else {
        SetDlgItemText(hWnd, IDC_BTN_START, L"Start");
    }
}