BOOL WINAPI Checklist_Init(void);
void WINAPI Checklist_Term(void);


void WINAPI Checklist_OnInitDialog(HWND hwnd);
int WINAPI Checklist_AddString(HWND hwnd, char* ptszText, BOOL fCheck);
void WINAPI Checklist_InitFinish(HWND hwnd);
BOOL WINAPI Checklist_GetState(HWND hwnd, int iItem);
void WINAPI Checklist_OnDestroy(HWND hwnd);
void WINAPI Checklist_SetState(HWND hwnd, int iItem, BOOL bState);
BOOL WINAPI Checklist_IsAutomatic(HWND hwnd, int iItem);
BOOL WINAPI Checklist_IsChecked(HWND hwnd, int iItem);
