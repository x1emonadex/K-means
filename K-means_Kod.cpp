#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cmath>
#include <random>
#include <algorithm>
#include <limits>
#include <iomanip>
#include <chrono>

#pragma comment(lib, "comctl32.lib")

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

#define ID_LOAD_BUTTON 1001
#define ID_CALC_BUTTON 1002
#define ID_K_INPUT 1003
#define ID_THRESHOLD_INPUT 1004
#define ID_MAX_ITER_INPUT 1005
#define ID_EXPORT_BUTTON 1007
#define ID_CLEAR_BUTTON 1008
#define ID_RESULTS_LIST 1009
#define ID_CLUSTERS_LIST 1010
#define ID_POINTS_LIST 1011
#define ID_STATS_LIST 1012

// Глобальные переменные
HINSTANCE hInst;
WCHAR szTitle[] = L"Анализатор кластеризации K-means - Профессиональная версия";
WCHAR szWindowClass[] = L"KmeansAnalyzerClass";
OPENFILENAME ofn;
wchar_t szFile[MAX_PATH] = L"";
wchar_t szSaveFile[MAX_PATH] = L"";

// Шрифты - ДОБАВЛЕН hFontRange
HFONT hFontTitle, hFontButton, hFontText, hFontBold, hFontRange;

// Структуры данных
struct Point {
    std::vector<double> coords;
    int clusterLabel = -1;
    int originalIndex = 0;
};

struct ClusterInfo {
    std::vector<double> centroid;
    int pointCount = 0;
    double avgDistance = 0.0;
    double minDistance = std::numeric_limits<double>::max();
    double maxDistance = 0.0;
    double variance = 0.0;
    double compactness = 0.0;
};

// Данные
std::vector<Point> points;
std::vector<ClusterInfo> clusters;
int k = 3;
int maxIterations = 100;
double convergenceThreshold = 0.01;
int currentIteration = 0;
bool isConverged = false;
double totalSSE = 0.0;
std::mt19937 rng(std::random_device{}());
std::chrono::steady_clock::time_point startTime;

// UI элементы - ДОБАВЛЕНЫ переменные для диапазонов
HWND hLoadBtn, hCalcBtn, hExportBtn, hClearBtn;
HWND hKInput, hThresholdInput, hMaxIterInput;
HWND hResultsList, hClustersList, hPointsList, hStatsList;
HWND hStatusText, hProgressBar;
HWND hTabControl;
HWND hKRangeLabel, hThresholdRange, hIterRange;

// Функции
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void InitializeResources();
void CleanupResources();
void CreateInterface(HWND hWnd);
void ResizeInterface(HWND hWnd, int width, int height);
HWND CreateListView(HWND parent, int x, int y, int w, int h, int id);
void AddListColumn(HWND hList, LPCWSTR text, int width, int index);
void AddListItem(HWND hList, int item, int subitem, LPCWSTR text);
void ClearList(HWND hList);
void CreateTabControl(HWND hWnd);
void ShowTabContent(int tabIndex);
void UpdateKRangeLabel();
void AutoResizeColumns(HWND hListView); // ДОБАВЛЕНА новая функция

// Отображение данных
void ShowWelcomeScreen();
void ShowDataLoadedInfo();
void ShowClusteringResults();
void UpdateStatus(const std::wstring& message);
void PopulateResultsTable();
void PopulateClustersTable();
void PopulatePointsTable();
void PopulateStatsTable();

// K-means алгоритм
void LoadDataFile();
void RunClusteringAnalysis();
void InitializeCentroidsKMeansPlusPlus();
double CalculateEuclideanDistance(const std::vector<double>& a, const std::vector<double>& b);
void AssignPointsToClusters();
void UpdateClusterCentroids();
void CalculateDetailedStatistics();
bool ExportDetailedResults();
void ClearAllData();

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow)
{
    hInst = hInstance;

    // Инициализация Common Controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES | ICC_PROGRESS_CLASS | ICC_TAB_CLASSES;
    InitCommonControlsEx(&icex);

    WNDCLASSEXW wcex = { sizeof(WNDCLASSEX) };
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInst;
    wcex.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = szWindowClass;

    RegisterClassExW(&wcex);

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, 1600, 1000, nullptr, nullptr, hInst, nullptr);

    if (!hWnd) return FALSE;

    InitializeResources();
    CreateInterface(hWnd);
    ShowWelcomeScreen();

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CleanupResources();
    return (int)msg.wParam;
}

void InitializeResources()
{
    hFontTitle = CreateFontW(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

    hFontButton = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

    hFontText = CreateFontW(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

    hFontBold = CreateFontW(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

    // ДОБАВЛЕН новый шрифт для диапазонов - жирный и увеличенный
    hFontRange = CreateFontW(14, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
}

void CleanupResources()
{
    DeleteObject(hFontTitle);
    DeleteObject(hFontButton);
    DeleteObject(hFontText);
    DeleteObject(hFontBold);
    DeleteObject(hFontRange); // ДОБАВЛЕНО удаление нового шрифта
}

void CreateInterface(HWND hWnd)
{
    // Заголовок
    CreateWindowW(L"STATIC", L"🔬 ПРОФЕССИОНАЛЬНЫЙ АНАЛИЗАТОР КЛАСТЕРИЗАЦИИ K-MEANS",
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        20, 20, 1540, 40, hWnd, nullptr, hInst, nullptr);

    // === ЛЕВАЯ ПАНЕЛЬ УПРАВЛЕНИЯ ===

    // Группа загрузки данных
    CreateWindowW(L"BUTTON", L"1. ЗАГРУЗКА ДАННЫХ",
        WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
        20, 80, 400, 100, hWnd, nullptr, hInst, nullptr);

    hLoadBtn = CreateWindowW(L"BUTTON", L"📁 Загрузить CSV файл",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        40, 110, 180, 35, hWnd, (HMENU)ID_LOAD_BUTTON, hInst, nullptr);

    hClearBtn = CreateWindowW(L"BUTTON", L"🗑️ Очистить данные",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        240, 110, 160, 35, hWnd, (HMENU)ID_CLEAR_BUTTON, hInst, nullptr);

    // Группа параметров
    CreateWindowW(L"BUTTON", L"2. ПАРАМЕТРЫ КЛАСТЕРИЗАЦИИ",
        WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
        20, 200, 400, 160, hWnd, nullptr, hInst, nullptr);

    CreateWindowW(L"STATIC", L"Количество кластеров (k):",
        WS_VISIBLE | WS_CHILD,
        40, 230, 180, 20, hWnd, nullptr, hInst, nullptr);
    hKInput = CreateWindowW(L"EDIT", L"3",
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
        230, 228, 60, 25, hWnd, (HMENU)ID_K_INPUT, hInst, nullptr);

    hKRangeLabel = CreateWindowW(L"STATIC", L"(2-8)",
        WS_VISIBLE | WS_CHILD,
        300, 230, 80, 20, hWnd, nullptr, hInst, nullptr);

    CreateWindowW(L"STATIC", L"Порог сходимости:",
        WS_VISIBLE | WS_CHILD,
        40, 260, 150, 20, hWnd, nullptr, hInst, nullptr);
    hThresholdInput = CreateWindowW(L"EDIT", L"0.01",
        WS_VISIBLE | WS_CHILD | WS_BORDER,
        200, 258, 80, 25, hWnd, (HMENU)ID_THRESHOLD_INPUT, hInst, nullptr);

    // ДОБАВЛЕНО создание переменной для диапазона порога сходимости
    hThresholdRange = CreateWindowW(L"STATIC", L"(0.001-0.1)",
        WS_VISIBLE | WS_CHILD,
        290, 260, 80, 20, hWnd, nullptr, hInst, nullptr);

    CreateWindowW(L"STATIC", L"Максимум итераций:",
        WS_VISIBLE | WS_CHILD,
        40, 290, 150, 20, hWnd, nullptr, hInst, nullptr);
    hMaxIterInput = CreateWindowW(L"EDIT", L"100",
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
        200, 288, 80, 25, hWnd, (HMENU)ID_MAX_ITER_INPUT, hInst, nullptr);

    // ДОБАВЛЕНО создание переменной для диапазона итераций
    hIterRange = CreateWindowW(L"STATIC", L"(5-500)",
        WS_VISIBLE | WS_CHILD,
        290, 290, 80, 20, hWnd, nullptr, hInst, nullptr);

    // Группа выполнения
    CreateWindowW(L"BUTTON", L"3. ВЫПОЛНЕНИЕ АНАЛИЗА",
        WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
        20, 380, 400, 120, hWnd, nullptr, hInst, nullptr);

    hCalcBtn = CreateWindowW(L"BUTTON", L"🚀 ЗАПУСТИТЬ КЛАСТЕРИЗАЦИЮ",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        40, 410, 360, 45, hWnd, (HMENU)ID_CALC_BUTTON, hInst, nullptr);

    // Прогресс-бар
    hProgressBar = CreateWindowW(PROGRESS_CLASS, nullptr,
        WS_VISIBLE | WS_CHILD | PBS_SMOOTH,
        40, 465, 360, 20, hWnd, nullptr, hInst, nullptr);

    // Группа экспорта
    CreateWindowW(L"BUTTON", L"4. ЭКСПОРТ РЕЗУЛЬТАТОВ",
        WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
        20, 520, 400, 80, hWnd, nullptr, hInst, nullptr);

    hExportBtn = CreateWindowW(L"BUTTON", L"💾 Экспорт полного отчета",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        40, 550, 360, 35, hWnd, (HMENU)ID_EXPORT_BUTTON, hInst, nullptr);

    // === ПРАВАЯ ПАНЕЛЬ РЕЗУЛЬТАТОВ ===

    // Заголовок результатов
    CreateWindowW(L"STATIC", L"📊 РЕЗУЛЬТАТЫ АНАЛИЗА",
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        450, 80, 1100, 30, hWnd, nullptr, hInst, nullptr);

    // Статус
    CreateWindowW(L"STATIC", L"Статус:",
        WS_VISIBLE | WS_CHILD,
        450, 120, 60, 20, hWnd, nullptr, hInst, nullptr);
    hStatusText = CreateWindowW(L"EDIT", L"",
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | ES_READONLY | WS_VSCROLL,
        520, 120, 1030, 60, hWnd, nullptr, hInst, nullptr);

    // Создаем вкладки
    CreateTabControl(hWnd);

    // Устанавливаем шрифты
    SendMessage(hLoadBtn, WM_SETFONT, (WPARAM)hFontButton, TRUE);
    SendMessage(hClearBtn, WM_SETFONT, (WPARAM)hFontButton, TRUE);
    SendMessage(hCalcBtn, WM_SETFONT, (WPARAM)hFontButton, TRUE);
    SendMessage(hExportBtn, WM_SETFONT, (WPARAM)hFontButton, TRUE);
    SendMessage(hKInput, WM_SETFONT, (WPARAM)hFontText, TRUE);
    SendMessage(hThresholdInput, WM_SETFONT, (WPARAM)hFontText, TRUE);
    SendMessage(hMaxIterInput, WM_SETFONT, (WPARAM)hFontText, TRUE);
    SendMessage(hStatusText, WM_SETFONT, (WPARAM)hFontText, TRUE);

    // ИСПРАВЛЕНО: Применяем новый жирный шрифт ко всем диапазонам
    SendMessage(hKRangeLabel, WM_SETFONT, (WPARAM)hFontRange, TRUE);
    SendMessage(hThresholdRange, WM_SETFONT, (WPARAM)hFontRange, TRUE);
    SendMessage(hIterRange, WM_SETFONT, (WPARAM)hFontRange, TRUE);
}

void UpdateKRangeLabel()
{
    if (points.empty()) {
        SetWindowTextW(hKRangeLabel, L"(min - 2)");
    }
    else {
        std::wstring rangeText = L"(2-" + std::to_wstring(points.size()) + L")";
        SetWindowTextW(hKRangeLabel, rangeText.c_str());
    }
}

// ИСПРАВЛЕНА функция CreateTabControl с правильными размерами столбцов
void CreateTabControl(HWND hWnd)
{
    // Создаем Tab Control
    hTabControl = CreateWindowW(WC_TABCONTROL, L"",
        WS_VISIBLE | WS_CHILD | TCS_TABS,
        450, 200, 1100, 750, hWnd, nullptr, hInst, nullptr);

    if (!hTabControl) {
        MessageBoxW(hWnd, L"Ошибка создания вкладок", L"Ошибка", MB_OK | MB_ICONERROR);
        return;
    }

    // Добавляем вкладки
    TCITEM tie;
    tie.mask = TCIF_TEXT;

    tie.pszText = (LPWSTR)L"📋 Общая информация";
    TabCtrl_InsertItem(hTabControl, 0, &tie);

    tie.pszText = (LPWSTR)L"🎯 Кластеры";
    TabCtrl_InsertItem(hTabControl, 1, &tie);

    tie.pszText = (LPWSTR)L"📍 Точки данных";
    TabCtrl_InsertItem(hTabControl, 2, &tie);

    tie.pszText = (LPWSTR)L"📊 Статистика";
    TabCtrl_InsertItem(hTabControl, 3, &tie);

    // Создаем таблицы для каждой вкладки
    RECT tabRect;
    GetClientRect(hTabControl, &tabRect);
    TabCtrl_AdjustRect(hTabControl, FALSE, &tabRect);

    // Таблица общих результатов
    hResultsList = CreateListView(hTabControl, tabRect.left + 10, tabRect.top + 10,
        tabRect.right - tabRect.left - 20, tabRect.bottom - tabRect.top - 20, ID_RESULTS_LIST);
    AddListColumn(hResultsList, L"Параметр", 200, 0);
    AddListColumn(hResultsList, L"Значение", 150, 1);
    AddListColumn(hResultsList, L"Описание", 700, 2);

    // Таблица кластеров - ИСПРАВЛЕНЫ РАЗМЕРЫ СТОЛБЦОВ
    hClustersList = CreateListView(hTabControl, tabRect.left + 10, tabRect.top + 10,
        tabRect.right - tabRect.left - 20, tabRect.bottom - tabRect.top - 20, ID_CLUSTERS_LIST);
    AddListColumn(hClustersList, L"Кластер", 80, 0);
    AddListColumn(hClustersList, L"Точек", 80, 1);
    AddListColumn(hClustersList, L"Процент", 80, 2);
    AddListColumn(hClustersList, L"Центроид", 350, 3);  // Уменьшено с 500 до 350
    AddListColumn(hClustersList, L"Ср. расстояние", 120, 4);
    AddListColumn(hClustersList, L"Компактность", 120, 5);
    AddListColumn(hClustersList, L"Качество", 130, 6);

    // Таблица точек - ИСПРАВЛЕНЫ РАЗМЕРЫ СТОЛБЦОВ ДЛЯ ПРАВИЛЬНОГО ОТОБРАЖЕНИЯ
    hPointsList = CreateListView(hTabControl, tabRect.left + 10, tabRect.top + 10,
        tabRect.right - tabRect.left - 20, tabRect.bottom - tabRect.top - 20, ID_POINTS_LIST);
    AddListColumn(hPointsList, L"Точка", 80, 0);
    AddListColumn(hPointsList, L"Кластер", 80, 1);
    AddListColumn(hPointsList, L"Координаты", 400, 2);  // ИСПРАВЛЕНО: уменьшено с 1200 до 400
    AddListColumn(hPointsList, L"Расстояние до центроида", 180, 3);
    AddListColumn(hPointsList, L"Статус", 150, 4);

    // Таблица статистики
    hStatsList = CreateListView(hTabControl, tabRect.left + 10, tabRect.top + 10,
        tabRect.right - tabRect.left - 20, tabRect.bottom - tabRect.top - 20, ID_STATS_LIST);
    AddListColumn(hStatsList, L"Метрика", 200, 0);
    AddListColumn(hStatsList, L"Значение", 150, 1);
    AddListColumn(hStatsList, L"Интерпретация", 700, 2);

    // Устанавливаем шрифты
    SendMessage(hResultsList, WM_SETFONT, (WPARAM)hFontText, TRUE);
    SendMessage(hClustersList, WM_SETFONT, (WPARAM)hFontText, TRUE);
    SendMessage(hPointsList, WM_SETFONT, (WPARAM)hFontText, TRUE);
    SendMessage(hStatsList, WM_SETFONT, (WPARAM)hFontText, TRUE);

    // Показываем первую вкладку
    ShowTabContent(0);
}

// ИСПРАВЛЕНА функция ResizeInterface с улучшенной обработкой размеров
void ResizeInterface(HWND hWnd, int width, int height)
{
    if (width < 1400) width = 1400;
    if (height < 900) height = 900;

    // Изменяем размер TabControl
    if (hTabControl)
        SetWindowPos(hTabControl, NULL, 450, 200, width - 480, height - 240, SWP_NOZORDER);

    // Изменяем размер поля статуса
    if (hStatusText)
        SetWindowPos(hStatusText, NULL, 520, 120, width - 550, 60, SWP_NOZORDER);

    // Обновляем размеры таблиц внутри вкладок
    if (hTabControl) {
        RECT tabRect;
        GetClientRect(hTabControl, &tabRect);
        TabCtrl_AdjustRect(hTabControl, FALSE, &tabRect);

        int tableWidth = tabRect.right - tabRect.left - 20;
        int tableHeight = tabRect.bottom - tabRect.top - 20;

        // Убеждаемся, что размеры положительные
        if (tableWidth > 0 && tableHeight > 0) {
            if (hResultsList)
                SetWindowPos(hResultsList, NULL, tabRect.left + 10, tabRect.top + 10,
                    tableWidth, tableHeight, SWP_NOZORDER);
            if (hClustersList)
                SetWindowPos(hClustersList, NULL, tabRect.left + 10, tabRect.top + 10,
                    tableWidth, tableHeight, SWP_NOZORDER);
            if (hPointsList)
                SetWindowPos(hPointsList, NULL, tabRect.left + 10, tabRect.top + 10,
                    tableWidth, tableHeight, SWP_NOZORDER);
            if (hStatsList)
                SetWindowPos(hStatsList, NULL, tabRect.left + 10, tabRect.top + 10,
                    tableWidth, tableHeight, SWP_NOZORDER);
        }
    }
}

void ShowTabContent(int tabIndex)
{
    // Скрываем все таблицы
    ShowWindow(hResultsList, SW_HIDE);
    ShowWindow(hClustersList, SW_HIDE);
    ShowWindow(hPointsList, SW_HIDE);
    ShowWindow(hStatsList, SW_HIDE);

    // Показываем нужную таблицу
    switch (tabIndex)
    {
    case 0: ShowWindow(hResultsList, SW_SHOW); break;
    case 1: ShowWindow(hClustersList, SW_SHOW); break;
    case 2: ShowWindow(hPointsList, SW_SHOW); break;
    case 3: ShowWindow(hStatsList, SW_SHOW); break;
    }
}

// ИСПРАВЛЕНА функция CreateListView с правильными скроллбарами
HWND CreateListView(HWND parent, int x, int y, int w, int h, int id)
{
    HWND hList = CreateWindowW(WC_LISTVIEW, L"",
        WS_VISIBLE | WS_CHILD | WS_BORDER | LVS_REPORT | LVS_SINGLESEL |
        LVS_SHOWSELALWAYS | WS_VSCROLL | WS_HSCROLL,  // ОСТАВЛЕНЫ ОБА СКРОЛЛБАРА
        x, y, w, h, parent, (HMENU)id, hInst, nullptr);

    // Устанавливаем расширенные стили для лучшего отображения
    ListView_SetExtendedListViewStyle(hList,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP);

    return hList;
}

// ДОБАВЛЕНА новая функция для автоматического изменения размера столбцов
void AutoResizeColumns(HWND hListView)
{
    int columnCount = Header_GetItemCount(ListView_GetHeader(hListView));

    for (int i = 0; i < columnCount; i++) {
        ListView_SetColumnWidth(hListView, i, LVSCW_AUTOSIZE_USEHEADER);

        // Получаем ширину после автоматического изменения размера
        int autoWidth = ListView_GetColumnWidth(hListView, i);

        // Устанавливаем минимальную ширину
        int minWidth = 80;
        if (autoWidth < minWidth) {
            ListView_SetColumnWidth(hListView, i, minWidth);
        }
    }
}

void AddListColumn(HWND hList, LPCWSTR text, int width, int index)
{
    LVCOLUMN lvc = { 0 };
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    lvc.pszText = (LPWSTR)text;
    lvc.cx = width;
    lvc.iSubItem = index;
    ListView_InsertColumn(hList, index, &lvc);
}

void AddListItem(HWND hList, int item, int subitem, LPCWSTR text)
{
    LVITEM lvi = { 0 };
    lvi.mask = LVIF_TEXT;
    lvi.iItem = item;
    lvi.iSubItem = subitem;
    lvi.pszText = (LPWSTR)text;

    if (subitem == 0) {
        ListView_InsertItem(hList, &lvi);
    }
    else {
        ListView_SetItem(hList, &lvi);
    }
}

void ClearList(HWND hList)
{
    ListView_DeleteAllItems(hList);
}

void ShowWelcomeScreen()
{
    UpdateStatus(L"Добро пожаловать в профессиональный анализатор кластеризации K-means!\r\n\r\nДля начала работы:\r\n1. Загрузите CSV файл с данными\r\n2. Настройте параметры\r\n3. Запустите анализ");

    // Заполняем таблицу общей информации
    ClearList(hResultsList);
    AddListItem(hResultsList, 0, 0, L"Статус системы");
    AddListItem(hResultsList, 0, 1, L"Готов к работе");
    AddListItem(hResultsList, 0, 2, L"Система готова к загрузке и анализу данных");

    AddListItem(hResultsList, 1, 0, L"Алгоритм");
    AddListItem(hResultsList, 1, 1, L"K-means++");
    AddListItem(hResultsList, 1, 2, L"Улучшенная инициализация центроидов для лучших результатов");

    AddListItem(hResultsList, 2, 0, L"Поддерживаемые форматы");
    AddListItem(hResultsList, 2, 1, L"CSV, TXT");
    AddListItem(hResultsList, 2, 2, L"Файлы с числовыми данными, разделенными запятыми или точкой с запятой");

    ClearList(hClustersList);
    ClearList(hPointsList);
    ClearList(hStatsList);

    UpdateKRangeLabel();
}

void ShowDataLoadedInfo()
{
    std::wstringstream status;
    status << L"Данные успешно загружены!\r\n\r\n";
    status << L"Количество точек: " << points.size() << L"\r\n";
    status << L"Размерность: " << (points.empty() ? 0 : points[0].coords.size()) << L"\r\n";
    status << L"Готов к кластеризации!";
    UpdateStatus(status.str());

    PopulateResultsTable();
    PopulatePointsTable();
    UpdateKRangeLabel();
}

void ShowClusteringResults()
{
    std::wstringstream status;
    status << L"Кластеризация завершена!\r\n\r\n";
    status << L"Выполнено итераций: " << currentIteration << L"/" << maxIterations << L"\r\n";
    status << L"Сходимость: " << (isConverged ? L"Достигнута" : L"Не достигнута") << L"\r\n";
    status << L"Общая ошибка SSE: " << std::fixed << std::setprecision(4) << totalSSE;
    UpdateStatus(status.str());

    PopulateResultsTable();
    PopulateClustersTable();
    PopulatePointsTable();
    PopulateStatsTable();
}

void UpdateStatus(const std::wstring& message)
{
    SetWindowTextW(hStatusText, message.c_str());
}

void PopulateResultsTable()
{
    ClearList(hResultsList);

    if (clusters.empty()) {
        AddListItem(hResultsList, 0, 0, L"Статус данных");
        AddListItem(hResultsList, 0, 1, L"Загружены");
        AddListItem(hResultsList, 0, 2, L"Данные успешно загружены и готовы для анализа");

        std::wstring pointsStr = std::to_wstring(points.size());
        AddListItem(hResultsList, 1, 0, L"Количество точек");
        AddListItem(hResultsList, 1, 1, pointsStr.c_str());
        AddListItem(hResultsList, 1, 2, L"Общее количество точек данных для кластеризации");

        std::wstring dimsStr = std::to_wstring(points.empty() ? 0 : points[0].coords.size());
        AddListItem(hResultsList, 2, 0, L"Размерность данных");
        AddListItem(hResultsList, 2, 1, dimsStr.c_str());
        AddListItem(hResultsList, 2, 2, L"Количество измерений (признаков) каждой точки");
    }
    else {
        AddListItem(hResultsList, 0, 0, L"Статус кластеризации");
        AddListItem(hResultsList, 0, 1, L"Завершена");
        AddListItem(hResultsList, 0, 2, L"Алгоритм K-means успешно выполнен");

        std::wstring iterStr = std::to_wstring(currentIteration) + L"/" + std::to_wstring(maxIterations);
        AddListItem(hResultsList, 1, 0, L"Итерации");
        AddListItem(hResultsList, 1, 1, iterStr.c_str());
        AddListItem(hResultsList, 1, 2, L"Количество выполненных итераций алгоритма");

        AddListItem(hResultsList, 2, 0, L"Сходимость");
        AddListItem(hResultsList, 2, 1, isConverged ? L"Достигнута" : L"Не достигнута");
        AddListItem(hResultsList, 2, 2, isConverged ? L"Алгоритм сошелся к оптимальному решению" : L"Требуется больше итераций");

        std::wstring kStr = std::to_wstring(k);
        AddListItem(hResultsList, 3, 0, L"Количество кластеров");
        AddListItem(hResultsList, 3, 1, kStr.c_str());
        AddListItem(hResultsList, 3, 2, L"Заданное количество кластеров для анализа");

        std::wstringstream sseStr;
        sseStr << std::fixed << std::setprecision(6) << totalSSE;
        AddListItem(hResultsList, 4, 0, L"Общая ошибка (SSE)");
        AddListItem(hResultsList, 4, 1, sseStr.str().c_str());
        AddListItem(hResultsList, 4, 2, L"Сумма квадратов расстояний до центроидов (чем меньше, тем лучше)");
    }
}

void PopulateClustersTable()
{
    ClearList(hClustersList);

    for (int i = 0; i < k; i++) {
        std::wstring clusterNum = std::to_wstring(i + 1);
        std::wstring pointCount = std::to_wstring(clusters[i].pointCount);

        double percentage = (clusters[i].pointCount * 100.0) / points.size();
        std::wstringstream percentStr;
        percentStr << std::fixed << std::setprecision(1) << percentage << L"%";

        std::wstringstream centroidStr;
        centroidStr << L"(";
        for (size_t j = 0; j < clusters[i].centroid.size(); j++) {
            centroidStr << std::fixed << std::setprecision(3) << clusters[i].centroid[j];
            if (j < clusters[i].centroid.size() - 1) centroidStr << L", ";
        }
        centroidStr << L")";

        std::wstringstream avgDistStr;
        avgDistStr << std::fixed << std::setprecision(6) << clusters[i].avgDistance;

        std::wstringstream compactnessStr;
        compactnessStr << std::fixed << std::setprecision(1) << (clusters[i].compactness * 100) << L"%";

        std::wstring quality;
        if (clusters[i].compactness > 0.8) quality = L"Отличное";
        else if (clusters[i].compactness > 0.6) quality = L"Хорошее";
        else if (clusters[i].compactness > 0.4) quality = L"Среднее";
        else quality = L"Низкое";

        AddListItem(hClustersList, i, 0, clusterNum.c_str());
        AddListItem(hClustersList, i, 1, pointCount.c_str());
        AddListItem(hClustersList, i, 2, percentStr.str().c_str());
        AddListItem(hClustersList, i, 3, centroidStr.str().c_str());
        AddListItem(hClustersList, i, 4, avgDistStr.str().c_str());
        AddListItem(hClustersList, i, 5, compactnessStr.str().c_str());
        AddListItem(hClustersList, i, 6, quality.c_str());
    }
}

void PopulatePointsTable()
{
    ClearList(hPointsList);

    for (size_t i = 0; i < points.size(); i++) {
        std::wstring pointNum = std::to_wstring(i + 1);

        std::wstring clusterStr;
        if (points[i].clusterLabel >= 0) {
            clusterStr = std::to_wstring(points[i].clusterLabel + 1);
        }
        else {
            clusterStr = L"Не назначен";
        }

        std::wstringstream coordsStr;
        coordsStr << L"(";
        for (size_t j = 0; j < points[i].coords.size(); j++) {
            coordsStr << std::fixed << std::setprecision(3) << points[i].coords[j];
            if (j < points[i].coords.size() - 1) coordsStr << L", ";
        }
        coordsStr << L")";

        std::wstring distanceStr = L"-";
        std::wstring statusStr = L"Ожидает кластеризации";

        if (points[i].clusterLabel >= 0 && points[i].clusterLabel < (int)clusters.size()) {
            double distance = CalculateEuclideanDistance(points[i].coords, clusters[points[i].clusterLabel].centroid);
            std::wstringstream distStr;
            distStr << std::fixed << std::setprecision(6) << distance;
            distanceStr = distStr.str();

            if (distance < clusters[points[i].clusterLabel].avgDistance * 0.8) {
                statusStr = L"Близко к центру";
            }
            else if (distance > clusters[points[i].clusterLabel].avgDistance * 1.5) {
                statusStr = L"На периферии";
            }
            else {
                statusStr = L"Типичная точка";
            }
        }

        AddListItem(hPointsList, static_cast<int>(i), 0, pointNum.c_str());
        AddListItem(hPointsList, static_cast<int>(i), 1, clusterStr.c_str());
        AddListItem(hPointsList, static_cast<int>(i), 2, coordsStr.str().c_str());
        AddListItem(hPointsList, static_cast<int>(i), 3, distanceStr.c_str());
        AddListItem(hPointsList, static_cast<int>(i), 4, statusStr.c_str());
    }

    // ДОБАВЛЕНО: Добавляем пустые строки для возможности прокрутки до конца
    int emptyRowsCount = 20; // Количество пустых строк для прокрутки
    for (int emptyRow = 0; emptyRow < emptyRowsCount; emptyRow++) {
        int rowIndex = static_cast<int>(points.size()) + emptyRow;
        AddListItem(hPointsList, rowIndex, 0, L"");
        AddListItem(hPointsList, rowIndex, 1, L"");
        AddListItem(hPointsList, rowIndex, 2, L"");
        AddListItem(hPointsList, rowIndex, 3, L"");
        AddListItem(hPointsList, rowIndex, 4, L"");
    }

    // ДОБАВЛЕНО: Автоматическое изменение размера столбцов после заполнения
    // AutoResizeColumns(hPointsList);  // Раскомментируйте если хотите автоматический размер
}

void PopulateStatsTable()
{
    ClearList(hStatsList);

    int row = 0;

    std::wstringstream totalPointsStr;
    totalPointsStr << points.size();
    AddListItem(hStatsList, row, 0, L"Общее количество точек");
    AddListItem(hStatsList, row, 1, totalPointsStr.str().c_str());
    AddListItem(hStatsList, row++, 2, L"Количество точек данных, участвующих в кластеризации");

    double avgSSE = totalSSE / points.size();
    std::wstringstream avgSSEStr;
    avgSSEStr << std::fixed << std::setprecision(6) << avgSSE;
    AddListItem(hStatsList, row, 0, L"Средняя квадратичная ошибка");
    AddListItem(hStatsList, row, 1, avgSSEStr.str().c_str());

    std::wstring sseInterpretation;
    if (avgSSE < 1.0) sseInterpretation = L"Отличное качество кластеризации";
    else if (avgSSE < 5.0) sseInterpretation = L"Хорошее качество кластеризации";
    else if (avgSSE < 20.0) sseInterpretation = L"Удовлетворительное качество";
    else sseInterpretation = L"Низкое качество, рассмотрите изменение параметров";

    AddListItem(hStatsList, row++, 2, sseInterpretation.c_str());

    double avgClusterSize = (double)points.size() / k;
    bool balanced = true;
    for (const auto& cluster : clusters) {
        if (cluster.pointCount < avgClusterSize * 0.3 || cluster.pointCount > avgClusterSize * 2.0) {
            balanced = false;
            break;
        }
    }

    AddListItem(hStatsList, row, 0, L"Баланс кластеров");
    AddListItem(hStatsList, row, 1, balanced ? L"Сбалансированы" : L"Несбалансированы");
    AddListItem(hStatsList, row++, 2, balanced ? L"Кластеры имеют примерно равные размеры" : L"Размеры кластеров сильно различаются");

    AddListItem(hStatsList, row, 0, L"Сходимость алгоритма");
    AddListItem(hStatsList, row, 1, isConverged ? L"Достигнута" : L"Не достигнута");
    AddListItem(hStatsList, row++, 2, isConverged ? L"Алгоритм нашел стабильное решение" : L"Увеличьте количество итераций");
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SIZE:
        ResizeInterface(hWnd, LOWORD(lParam), HIWORD(lParam));
        break;

    case WM_NOTIFY:
    {
        LPNMHDR pnmh = (LPNMHDR)lParam;
        if (pnmh->hwndFrom == hTabControl && pnmh->code == TCN_SELCHANGE)
        {
            int selectedTab = TabCtrl_GetCurSel(hTabControl);
            ShowTabContent(selectedTab);
        }
        break;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_LOAD_BUTTON:
            LoadDataFile();
            break;
        case ID_CLEAR_BUTTON:
            ClearAllData();
            break;
        case ID_CALC_BUTTON:
            RunClusteringAnalysis();
            break;
        case ID_EXPORT_BUTTON:
            ExportDetailedResults();
            break;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Реализация функций K-means алгоритма

// ИСПРАВЛЕНА функция LoadDataFile с поддержкой разных разделителей
void LoadDataFile()
{
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetActiveWindow();
    ofn.lpstrFilter = L"CSV Files (*.csv)\0*.csv\0Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (GetOpenFileName(&ofn))
    {
        ClearAllData();
        SendMessage(hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
        SendMessage(hProgressBar, PBM_SETPOS, 10, 0);
        UpdateStatus(L"Загрузка файла...");

        std::wifstream file(szFile);
        std::wstring line;

        // Определяем разделитель автоматически
        wchar_t delimiter = L',';
        if (std::getline(file, line)) {
            // Проверяем, какой разделитель используется
            if (line.find(L';') != std::wstring::npos) {
                delimiter = L';';
            }
            else if (line.find(L',') != std::wstring::npos) {
                delimiter = L',';
            }
            else if (line.find(L'\t') != std::wstring::npos) {
                delimiter = L'\t';
            }

            // Проверяем, является ли первая строка заголовком
            bool isHeader = false;
            std::wstringstream ss(line);
            std::wstring token;
            while (std::getline(ss, token, delimiter)) {
                // Удаляем пробелы
                token.erase(std::remove_if(token.begin(), token.end(), ::iswspace), token.end());
                if (!token.empty()) {
                    try {
                        std::stod(token);
                    }
                    catch (...) {
                        isHeader = true;
                        break;
                    }
                }
            }

            // Если не заголовок, возвращаемся к началу файла
            if (!isHeader) {
                file.seekg(0);
            }
        }

        // Читаем данные
        while (std::getline(file, line))
        {
            // Пропускаем пустые строки
            if (line.empty()) continue;

            std::wstringstream ss(line);
            std::wstring token;
            Point p;
            p.originalIndex = static_cast<int>(points.size());

            // Парсим все столбцы с правильным разделителем
            while (std::getline(ss, token, delimiter))
            {
                try {
                    // Удаляем все пробельные символы
                    token.erase(std::remove_if(token.begin(), token.end(), ::iswspace), token.end());

                    if (!token.empty()) {
                        // Заменяем запятую на точку для десятичных чисел (если нужно)
                        std::replace(token.begin(), token.end(), L',', L'.');

                        double value = std::stod(token);
                        if (std::isfinite(value)) {
                            p.coords.push_back(value);
                        }
                    }
                }
                catch (const std::exception&) {
                    // Пропускаем некорректные значения, но продолжаем обработку строки
                    continue;
                }
            }

            // Добавляем точку только если у неё есть координаты
            if (!p.coords.empty()) {
                points.push_back(p);
            }
        }

        file.close();
        SendMessage(hProgressBar, PBM_SETPOS, 100, 0);

        if (!points.empty()) {
            // Проверяем, что все точки имеют одинаковое количество измерений
            size_t expectedDimensions = points[0].coords.size();
            bool dimensionsConsistent = true;

            for (const auto& point : points) {
                if (point.coords.size() != expectedDimensions) {
                    dimensionsConsistent = false;
                    break;
                }
            }

            if (dimensionsConsistent) {
                std::wstringstream successMsg;
                successMsg << L"Данные успешно загружены!\r\n\r\n";
                successMsg << L"Количество точек: " << points.size() << L"\r\n";
                successMsg << L"Размерность: " << expectedDimensions << L"\r\n";
                successMsg << L"Разделитель: " << (delimiter == L';' ? L"точка с запятой (;)" :
                    delimiter == L',' ? L"запятая (,)" : L"табуляция") << L"\r\n";
                successMsg << L"Готов к кластеризации!";
                UpdateStatus(successMsg.str());

                ShowDataLoadedInfo();
            }
            else {
                UpdateStatus(L"Предупреждение: Обнаружены строки с разным количеством столбцов.\r\nНекоторые данные могут быть обработаны некорректно.");
                ShowDataLoadedInfo();
            }
        }
        else {
            std::wstringstream errorMsg;
            errorMsg << L"Ошибка: В файле не найдены корректные числовые данные.\r\n\r\n";
            errorMsg << L"Проверьте формат файла:\r\n";
            errorMsg << L"- Используемый разделитель: " << (delimiter == L';' ? L";" : delimiter == L',' ? L"," : L"табуляция") << L"\r\n";
            errorMsg << L"- Убедитесь, что файл содержит только числовые данные\r\n";
            errorMsg << L"- Проверьте кодировку файла";
            UpdateStatus(errorMsg.str());
            SendMessage(hProgressBar, PBM_SETPOS, 0, 0);
        }
    }
}

void RunClusteringAnalysis()
{
    if (points.empty()) {
        UpdateStatus(L"Ошибка: Сначала загрузите данные!");
        return;
    }

    wchar_t buffer[32];
    GetWindowTextW(hKInput, buffer, 32);
    k = _wtoi(buffer);

    GetWindowTextW(hThresholdInput, buffer, 32);
    convergenceThreshold = _wtof(buffer);

    GetWindowTextW(hMaxIterInput, buffer, 32);
    maxIterations = _wtoi(buffer);

    if (k < 1 || k > static_cast<int>(points.size())) {
        std::wstringstream errorMsg;
        errorMsg << L"Ошибка: Количество кластеров должно быть от 1 до " << points.size() << L" (количество точек данных).";
        UpdateStatus(errorMsg.str());
        return;
    }

    if (convergenceThreshold <= 0 || convergenceThreshold >= 1) {
        UpdateStatus(L"Ошибка: Порог сходимости должен быть между 0 и 1.");
        return;
    }

    startTime = std::chrono::steady_clock::now();
    SendMessage(hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, maxIterations));
    UpdateStatus(L"Инициализация центроидов...");

    InitializeCentroidsKMeansPlusPlus();
    currentIteration = 0;
    isConverged = false;

    std::vector<std::vector<double>> prevCentroids;

    while (currentIteration < maxIterations && !isConverged)
    {
        prevCentroids.clear();
        for (const auto& cluster : clusters) {
            prevCentroids.push_back(cluster.centroid);
        }

        AssignPointsToClusters();
        UpdateClusterCentroids();

        isConverged = true;
        for (int i = 0; i < k; i++) {
            if (CalculateEuclideanDistance(clusters[i].centroid, prevCentroids[i]) > convergenceThreshold) {
                isConverged = false;
                break;
            }
        }

        currentIteration++;
        SendMessage(hProgressBar, PBM_SETPOS, currentIteration, 0);

        if (currentIteration % 5 == 0) {
            std::wstringstream status;
            status << L"Выполнение кластеризации... Итерация " << currentIteration << L"/" << maxIterations;
            UpdateStatus(status.str());
        }
    }

    CalculateDetailedStatistics();
    ShowClusteringResults();
}

void InitializeCentroidsKMeansPlusPlus()
{
    clusters.clear();
    clusters.resize(k);

    std::uniform_int_distribution<size_t> dist(0, points.size() - 1);
    clusters[0].centroid = points[dist(rng)].coords;

    for (int i = 1; i < k; i++) {
        std::vector<double> distances(points.size());
        double totalDist = 0.0;

        for (size_t j = 0; j < points.size(); j++) {
            double minDist = std::numeric_limits<double>::max();
            for (int c = 0; c < i; c++) {
                double d = CalculateEuclideanDistance(points[j].coords, clusters[c].centroid);
                minDist = std::min(minDist, d);
            }
            distances[j] = minDist * minDist;
            totalDist += distances[j];
        }

        std::uniform_real_distribution<double> realDist(0.0, totalDist);
        double target = realDist(rng);
        double cumulative = 0.0;

        for (size_t j = 0; j < points.size(); j++) {
            cumulative += distances[j];
            if (cumulative >= target) {
                clusters[i].centroid = points[j].coords;
                break;
            }
        }
    }
}

double CalculateEuclideanDistance(const std::vector<double>& a, const std::vector<double>& b)
{
    if (a.size() != b.size()) return std::numeric_limits<double>::max();

    double sum = 0.0;
    for (size_t i = 0; i < a.size(); i++) {
        double diff = a[i] - b[i];
        sum += diff * diff;
    }
    return std::sqrt(sum);
}

void AssignPointsToClusters()
{
    for (auto& cluster : clusters) {
        cluster.pointCount = 0;
    }

    for (auto& point : points) {
        double minDist = std::numeric_limits<double>::max();
        int bestCluster = 0;

        for (int i = 0; i < k; i++) {
            double dist = CalculateEuclideanDistance(point.coords, clusters[i].centroid);
            if (dist < minDist) {
                minDist = dist;
                bestCluster = i;
            }
        }

        point.clusterLabel = bestCluster;
        clusters[bestCluster].pointCount++;
    }
}

void UpdateClusterCentroids()
{
    for (int i = 0; i < k; i++) {
        if (clusters[i].pointCount == 0) continue;

        std::vector<double> newCentroid(clusters[i].centroid.size(), 0.0);

        for (const auto& point : points) {
            if (point.clusterLabel == i) {
                for (size_t j = 0; j < point.coords.size(); j++) {
                    newCentroid[j] += point.coords[j];
                }
            }
        }

        for (size_t j = 0; j < newCentroid.size(); j++) {
            newCentroid[j] /= clusters[i].pointCount;
        }

        clusters[i].centroid = newCentroid;
    }
}

void CalculateDetailedStatistics()
{
    totalSSE = 0.0;

    for (int i = 0; i < k; i++) {
        if (clusters[i].pointCount == 0) {
            clusters[i].avgDistance = 0.0;
            clusters[i].minDistance = 0.0;
            clusters[i].maxDistance = 0.0;
            clusters[i].variance = 0.0;
            clusters[i].compactness = 0.0;
            continue;
        }

        double totalDist = 0.0;
        clusters[i].minDistance = std::numeric_limits<double>::max();
        clusters[i].maxDistance = 0.0;
        std::vector<double> distances;

        for (const auto& point : points) {
            if (point.clusterLabel == i) {
                double dist = CalculateEuclideanDistance(point.coords, clusters[i].centroid);
                totalDist += dist;
                distances.push_back(dist);
                clusters[i].minDistance = std::min(clusters[i].minDistance, dist);
                clusters[i].maxDistance = std::max(clusters[i].maxDistance, dist);
                totalSSE += dist * dist;
            }
        }

        clusters[i].avgDistance = totalDist / clusters[i].pointCount;

        double variance = 0.0;
        for (double dist : distances) {
            variance += (dist - clusters[i].avgDistance) * (dist - clusters[i].avgDistance);
        }
        clusters[i].variance = variance / distances.size();

        if (clusters[i].maxDistance > 0) {
            clusters[i].compactness = 1.0 - (clusters[i].avgDistance / clusters[i].maxDistance);
        }
        else {
            clusters[i].compactness = 1.0;
        }
    }
}

bool ExportDetailedResults()
{
    if (points.empty() || clusters.empty()) {
        UpdateStatus(L"Ошибка: Нет результатов для экспорта. Сначала выполните кластеризацию.");
        return false;
    }

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetActiveWindow();
    ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = szSaveFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L"txt";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    wcscpy_s(szSaveFile, L"kmeans_results.txt");

    if (GetSaveFileName(&ofn))
    {
        std::ofstream file(szSaveFile);
        if (!file.is_open()) {
            UpdateStatus(L"Ошибка: Не удалось создать файл для экспорта.");
            return false;
        }

        file << "=== ENHANCED K-MEANS CLUSTERING RESULTS - PERFECT EDITION ===\n\n";

        file << "Parameters:\n";
        file << "  Number of clusters (k): " << k << "\n";
        file << "  Convergence threshold: " << std::fixed << std::setprecision(2) << convergenceThreshold << "\n";
        file << "  Max iterations: " << maxIterations << "\n";
        file << "  Actual iterations: " << currentIteration << "\n\n";

        file << "Quality metrics:\n";
        file << "  SSE (Sum of Squared Errors): " << std::fixed << std::setprecision(6) << totalSSE << "\n\n";

        file << "Cluster assignments:\n";
        for (size_t i = 0; i < points.size(); i++) {
            file << "Point " << (i + 1) << ": Cluster " << (points[i].clusterLabel + 1) << " (";
            for (size_t j = 0; j < points[i].coords.size(); j++) {
                file << std::fixed << std::setprecision(3) << points[i].coords[j];
                if (j < points[i].coords.size() - 1) file << ", ";
            }
            file << ")\n";
        }

        file << "\nCluster centroids and statistics:\n";
        for (int i = 0; i < k; i++) {
            file << "Cluster " << (i + 1) << ":\n";
            file << "  Points: " << clusters[i].pointCount << "\n";
            file << "  Centroid: (";
            for (size_t j = 0; j < clusters[i].centroid.size(); j++) {
                file << std::fixed << std::setprecision(6) << clusters[i].centroid[j];
                if (j < clusters[i].centroid.size() - 1) file << ", ";
            }
            file << ")\n";
            file << "  Average intra-cluster distance: " << std::fixed << std::setprecision(6) << clusters[i].avgDistance << "\n\n";
        }

        file.close();

        UpdateStatus(L"Отчет успешно экспортирован!\r\n\r\nФайл: " + std::wstring(szSaveFile));
        return true;
    }

    return false;
}

void ClearAllData()
{
    points.clear();
    clusters.clear();
    currentIteration = 0;
    isConverged = false;
    totalSSE = 0.0;
    SendMessage(hProgressBar, PBM_SETPOS, 0, 0);
    ShowWelcomeScreen();
}
