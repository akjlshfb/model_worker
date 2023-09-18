#include <windows.h>
#include <stdio.h>

#define DEBUG_PRINT
#ifdef DEBUG_PRINT
    #include <conio.h>
#endif

int target_interval = 300; //seconds

#define ID_MENU_PAUSED 1
#define ID_MENU_START 2
#define ID_MENU_AUTO_LEAVE 3
#define ID_MENU_EXIT 4

char STR_MENU_PAUSE[] = "Pause";
char STR_MENU_PAUSED[] = "Paused";
char STR_MENU_START[] = "Start";
char STR_MENU_STARTED[] = "Started";
char STR_MENU_AUTO_LEAVE[] = "Auto leave";

LPCTSTR szAppName = TEXT("Model worker App");
LPCTSTR szWndName = TEXT("Model worker Window");
HMENU hmenu;//Menu handel

BOOL running = FALSE;
BOOL autoLeave = FALSE;
HANDLE runningStateMutex;

HANDLE mintThread;

MENUITEMINFOA newMenuItemInfo;

BOOL inRestTime() {
    SYSTEMTIME time;
    WORD day, hour, minute;
    BOOL shouldRest;

    GetLocalTime(&time);
    day = time.wDayOfWeek;
    hour = time.wHour;
    minute = time.wMinute;
    shouldRest = (
        (day >= 6) || //weekend
        ((day < 6) && (
            ((hour < 8) || ((hour == 8) && (minute < 30))) || //befor 08:30
            ((hour == 12) || ((hour == 13) && (minute < 30))) || //12:00 ~ 13:30
            (hour >= 18) //after 18:00
        ))
    );

    return shouldRest;
}

void setRunningMenuItem() { //thread unsafe
    newMenuItemInfo.fMask = MIIM_STRING | MIIM_STATE;
    newMenuItemInfo.dwTypeData = STR_MENU_STARTED;
    newMenuItemInfo.cch = sizeof(STR_MENU_STARTED);
    newMenuItemInfo.fState = MFS_CHECKED;
    SetMenuItemInfo(hmenu, ID_MENU_START, FALSE, &newMenuItemInfo);
    newMenuItemInfo.dwTypeData = STR_MENU_PAUSE;
    newMenuItemInfo.cch = sizeof(STR_MENU_PAUSE);
    newMenuItemInfo.fState = MFS_UNCHECKED;
    SetMenuItemInfo(hmenu, ID_MENU_PAUSED, FALSE, &newMenuItemInfo);
}

void setPausedMenuItem() { //thread unsafe
    newMenuItemInfo.fMask = MIIM_STRING | MIIM_STATE;
    newMenuItemInfo.dwTypeData = STR_MENU_START;
    newMenuItemInfo.cch = sizeof(STR_MENU_START);
    newMenuItemInfo.fState = MFS_UNCHECKED;
    SetMenuItemInfo(hmenu, ID_MENU_START, FALSE, &newMenuItemInfo);
    newMenuItemInfo.dwTypeData = STR_MENU_PAUSED;
    newMenuItemInfo.cch = sizeof(STR_MENU_PAUSED);
    newMenuItemInfo.fState = MFS_CHECKED;
    SetMenuItemInfo(hmenu, ID_MENU_PAUSED, FALSE, &newMenuItemInfo);
}

void setAutoLeaveMenuItem() { //thread unsafe
    if (autoLeave) {
        newMenuItemInfo.fMask = MIIM_STATE;
        newMenuItemInfo.fState = MFS_CHECKED;
        SetMenuItemInfo(hmenu, ID_MENU_AUTO_LEAVE, FALSE, &newMenuItemInfo);
    } else {
        newMenuItemInfo.fMask = MIIM_STATE;
        newMenuItemInfo.fState = MFS_UNCHECKED;
        SetMenuItemInfo(hmenu, ID_MENU_AUTO_LEAVE, FALSE, &newMenuItemInfo);
    }
}

void setMenuItemDisplayState() { //thread unsafe
    if (running) {
        setRunningMenuItem();
    } else {
        setPausedMenuItem();
    }
    setAutoLeaveMenuItem();
}

DWORD WINAPI mintProc(LPVOID lpParam) {
    DWORD mutexWaitResult;

    LASTINPUTINFO lastInput;
    DWORD nowTick;
    long tickElapsed, targetInterval, detectInterval, detectThreshold;
    BOOL parseFailed;

    //init
    targetInterval = target_interval; //seconds
    detectInterval = targetInterval * 2 * 1000 / 5; //detect = target / 2.5
    detectThreshold = detectInterval * 4 / 5; //threshold = detect * 0.8
    lastInput.cbSize = sizeof(LASTINPUTINFO);

    while (TRUE) {
        Sleep(detectInterval);

        mutexWaitResult = WaitForSingleObject(runningStateMutex, INFINITE);
        if (mutexWaitResult != WAIT_OBJECT_0) {
            continue;
        }

        if (autoLeave) {
            running = !inRestTime();
            setMenuItemDisplayState();
        }

        if (running) {

            nowTick = GetTickCount();
            GetLastInputInfo(&lastInput);
            tickElapsed = nowTick - lastInput.dwTime;
            if (tickElapsed > detectThreshold) {
                mouse_event(MOUSEEVENTF_MOVE, 0, 0, 0, 0);
            }
#ifdef DEBUG_PRINT
            printf("Running.\n");
            printf("Time elapsed since last input: %ld ms.", tickElapsed);
            if (tickElapsed > detectThreshold) {
                printf(" Idle detected! Update input time.");
            }
            printf("\n");
#endif
        } else {
#ifdef DEBUG_PRINT
            printf("Paused.\n");
            nowTick = GetTickCount();
            GetLastInputInfo(&lastInput);
            tickElapsed = nowTick - lastInput.dwTime;
            printf("Time elapsed since last input: %ld ms.\n", tickElapsed);
#endif
        }

        ReleaseMutex(runningStateMutex);
    }

    return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    NOTIFYICONDATA nid;
    UINT WM_TASKBARCREATED;
    POINT pt;
    int xx;
    DWORD mutexWaitResult;
 
    WM_TASKBARCREATED = RegisterWindowMessage(TEXT("TaskbarCreated"));
    switch (message) {
    case WM_CREATE:
        nid.cbSize = sizeof(nid);
        nid.hWnd = hwnd;
        nid.uID = 0;
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.uCallbackMessage = WM_USER;
        nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        lstrcpy(nid.szTip, szAppName);
        Shell_NotifyIcon(NIM_ADD, &nid);
        hmenu=CreatePopupMenu();
        AppendMenu(hmenu,MF_STRING,ID_MENU_PAUSED,STR_MENU_PAUSE);
        AppendMenu(hmenu,MF_STRING,ID_MENU_START,STR_MENU_START);
        AppendMenu(hmenu,MF_SEPARATOR,0,NULL);
        AppendMenu(hmenu,MF_STRING,ID_MENU_AUTO_LEAVE,STR_MENU_AUTO_LEAVE);
        AppendMenu(hmenu,MF_SEPARATOR,0,NULL);
        AppendMenu(hmenu,MF_STRING,ID_MENU_EXIT,"Exit");
        mutexWaitResult = WaitForSingleObject(runningStateMutex, INFINITE);
        if (mutexWaitResult == WAIT_OBJECT_0) {
            if (autoLeave && inRestTime()) {
                running = FALSE;
            }
            setMenuItemDisplayState();
            ReleaseMutex(runningStateMutex);
        }
        break;
    case WM_USER:
        if ((lParam == WM_RBUTTONDOWN) || (lParam == WM_LBUTTONDOWN)) {
            char temp[100];
            GetCursorPos(&pt);
            SetForegroundWindow(hwnd);
            xx=TrackPopupMenu(hmenu,TPM_RETURNCMD,pt.x,pt.y,0,hwnd,NULL);
            switch (xx) {
            case ID_MENU_PAUSED:
                mutexWaitResult = WaitForSingleObject(runningStateMutex, INFINITE);
                if (mutexWaitResult == WAIT_OBJECT_0) {
                    running = FALSE;
                    autoLeave = FALSE;
                    setMenuItemDisplayState();
                    ReleaseMutex(runningStateMutex);
                }
                break;
            case ID_MENU_START:
                mutexWaitResult = WaitForSingleObject(runningStateMutex, INFINITE);
                if (mutexWaitResult == WAIT_OBJECT_0) {
                    running = TRUE;
                    autoLeave = FALSE;
                    setMenuItemDisplayState();
                    ReleaseMutex(runningStateMutex);
                }
                break;
            case ID_MENU_AUTO_LEAVE:
                mutexWaitResult = WaitForSingleObject(runningStateMutex, INFINITE);
                if (mutexWaitResult == WAIT_OBJECT_0) {
                    autoLeave = !autoLeave;
                    if (autoLeave && inRestTime()) {
                        running = FALSE;
                    }
                    setMenuItemDisplayState();
                    ReleaseMutex(runningStateMutex);
                }
                break;
            case ID_MENU_EXIT:
                TerminateThread(mintThread, 0);
                SendMessage(hwnd, WM_CLOSE, wParam, lParam);
                break;
            default:
                break;
            }
        }
        break;
    case WM_DESTROY:
        Shell_NotifyIcon(NIM_DELETE, &nid);
        PostQuitMessage(0);
        break;
    default:
        //add icon after explorer.exe crash
        if (message == WM_TASKBARCREATED)
            SendMessage(hwnd, WM_CREATE, wParam, lParam);
        break;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}
 
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR szCmdLine, int iCmdShow) {
    HWND hwnd;
    MSG msg;
    WNDCLASS wndclass;
    DWORD threadId;

    char* ptr = szCmdLine;
    BOOL parseErr = FALSE;
    //parse args
    if (szCmdLine[0] != '\0') {
        while (*ptr == ' ') { ptr++; }
        if (*ptr != '\0') { //has first arg
            if (sscanf(ptr, "%d", &target_interval) == 1) { //first arg valid
                while ((*ptr != ' ') && (*ptr != '\0')) { ptr++; }
                while (*ptr == ' ') { ptr++; }
                if (*ptr != '\0') { //has second arg
                    parseErr = TRUE; //not implemented yet.
                }
            } else {
                parseErr = TRUE;
            }
        }
    }
    if (parseErr) {
        MessageBox(NULL, TEXT("Usage: model_worker.exe <target_detect_interval>"), szAppName, MB_ICONERROR);
        return -1;
    }

    //initialize mutexes and threads
    runningStateMutex = CreateMutex(NULL, FALSE, NULL);
    if (runningStateMutex == NULL) {
        MessageBox(NULL, TEXT("Create mutex failed."), szAppName, MB_ICONERROR);
        return -1;
    }

    newMenuItemInfo.cbSize = sizeof(MENUITEMINFO);
    newMenuItemInfo.fMask = 0; //items to be changed
    newMenuItemInfo.fType = MFT_STRING;
    newMenuItemInfo.fState = MFS_UNCHECKED; //change
    newMenuItemInfo.wID = 0;
    newMenuItemInfo.hSubMenu = NULL;
    newMenuItemInfo.hbmpChecked = NULL;
    newMenuItemInfo.hbmpUnchecked = NULL;
    newMenuItemInfo.dwItemData = 0;
    newMenuItemInfo.dwTypeData = NULL; //string
    newMenuItemInfo.cch = 0; //string length
    newMenuItemInfo.hbmpItem = NULL;

    running = TRUE;
    autoLeave = TRUE;

    mintThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)mintProc, NULL, 0, &threadId);
    if (mintThread == NULL) {
        MessageBox(NULL, TEXT("Create thread failed."), szAppName, MB_ICONERROR);
        return -1;
    }

#ifdef DEBUG_PRINT
    //debug
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
#endif

    //UI starts here
    HWND handle = FindWindow(NULL, szWndName);
    if (handle != NULL) {
        MessageBox(NULL, TEXT("Application is already running."), szAppName, MB_ICONERROR);
        return -1;
    }
 
    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = WndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = hInstance;
    wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = szAppName;
 
    if (!RegisterClass(&wndclass)) {
        MessageBox(NULL, TEXT("This program requires Windows NT!"), szAppName, MB_ICONERROR);
        return 0;
    }
 
    // hide btn
    hwnd = CreateWindowEx(WS_EX_TOOLWINDOW,
        szAppName, szWndName,
        WS_POPUP,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        NULL, NULL, hInstance, NULL);
 
    ShowWindow(hwnd, iCmdShow);
    UpdateWindow(hwnd);
 
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    //De-init
    WaitForSingleObject(mintThread, INFINITE);
    CloseHandle(mintThread);
    CloseHandle(runningStateMutex);
#ifdef DEBUG_PRINT
    fclose(stdout);
#endif

    return msg.wParam;
}