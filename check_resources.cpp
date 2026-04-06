#include <windows.h>
#include <iostream>

BOOL CALLBACK EnumResNameProc(HMODULE hModule, LPCSTR lpszType, LPSTR lpszName, LONG_PTR lParam) {
    if (IS_INTRESOURCE(lpszName)) {
        std::cout << "  Resource ID: " << (int)(intptr_t)lpszName << std::endl;
    } else {
        std::cout << "  Resource Name: " << lpszName << std::endl;
    }
    return TRUE;
}

BOOL CALLBACK EnumResTypeProc(HMODULE hModule, LPSTR lpszType, LONG_PTR lParam) {
    if (IS_INTRESOURCE(lpszType)) {
        std::cout << "Type ID: " << (int)(intptr_t)lpszType << std::endl;
    } else {
        std::cout << "Type: " << lpszType << std::endl;
    }
    EnumResourceNamesA(hModule, lpszType, EnumResNameProc, 0);
    return TRUE;
}

int main() {
    HMODULE hModule = GetModuleHandleA(NULL);
    if (!hModule) {
        std::cerr << "Failed to get module handle" << std::endl;
        return 1;
    }

    std::cout << "Enumerating resources in exe:" << std::endl;
    EnumResourceTypesA(hModule, EnumResTypeProc, 0);

    return 0;
}