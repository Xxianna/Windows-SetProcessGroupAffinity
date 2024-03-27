#include <Windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <Windows.h>
#include <tlhelp32.h>

using namespace std;

#define min(a,b)            (((a) < (b)) ? (a) : (b))



BOOL SetProcessGroupAffinity(HANDLE hProcess, const GROUP_AFFINITY& groupAffinity) {
    // 获取进程ID
    DWORD processId = GetProcessId(hProcess);
    if (processId == 0) {
        // 获取进程ID失败
        return FALSE;
    }

    // 创建系统快照
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        // 创建快照失败
        return FALSE;
    }

    THREADENTRY32 te;
    te.dwSize = sizeof(THREADENTRY32);

    // 遍历所有线程
    if (Thread32First(hSnapshot, &te)) {
        do {
            if (te.dwSize >= FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) + sizeof(te.th32OwnerProcessID)) {
                if (te.th32OwnerProcessID == processId) {
                    // 找到进程中的线程
                    HANDLE hThread = OpenThread(THREAD_SET_INFORMATION, FALSE, te.th32ThreadID);
                    if (hThread != NULL) {
                        // 设置线程的组亲和性
                        if (!SetThreadGroupAffinity(hThread, &groupAffinity, NULL)) {
                            // 设置失败
                            CloseHandle(hThread);
                            CloseHandle(hSnapshot);
                            return FALSE;
                        }
                        CloseHandle(hThread);
                    }
                }
            }
        } while (Thread32Next(hSnapshot, &te));
    }

    CloseHandle(hSnapshot);
    return TRUE;
}


bool SetThreadAffinityPerGroup(const std::vector<DWORD_PTR>& affinityPerGroup,const PROCESS_INFORMATION &pi){
    // HANDLE hThread = GetCurrentThread(); // 获取当前线程的句柄
    DWORD_PTR processAffinityMask, systemAffinityMask;
    GetProcessAffinityMask(GetCurrentProcess(), &processAffinityMask, &systemAffinityMask);

    // 遍历所有处理器组，并设置线程亲和性
    ULONG groupCount = 0;
    if (!GetNumaHighestNodeNumber (&groupCount))
    {
        cout<<"GetNumaHighestNodeNumber failed: "<<GetLastError();
        return 1;
    }
    ++groupCount; // GetNumaHighestNodeNumber 返回的是最高的节点编号，节点计数应该是编号+1
    static bool firsttime = true;
    if(firsttime){
        std::cout<< "[\033[32mINFO\033[0m]Core groups: "<<groupCount<<endl;
        firsttime = false;
    }

    for (WORD groupIndex = 0; groupIndex < min(groupCount,affinityPerGroup.size()); ++groupIndex) {
        GROUP_AFFINITY groupAffinity = {};
        if (groupIndex < affinityPerGroup.size()) {
            groupAffinity.Mask = affinityPerGroup[groupIndex];
            groupAffinity.Group = groupIndex;
            groupAffinity.Reserved[0] = 0;
            groupAffinity.Reserved[1] = 0;
            groupAffinity.Reserved[2] = 0;
            // 为当前线程设置亲和性
            // SetThreadGroupAffinity(pi.hThread, &groupAffinity, nullptr);
            // SetProcessAffinityMask(pi.hProcess, affinityPerGroup[groupIndex]);
            // if (!SetThreadGroupAffinity(pi.hProcess, &groupAffinity, nullptr)) {
            if (!SetProcessAffinityMask(pi.hProcess, affinityPerGroup[groupIndex])) {
                // 错误处理，例如记录日志或显示错误信息
                DWORD dwError = GetLastError();
                // 这里应该有错误处理代码
                return 1;
            }
        }
    }
    return 0;
}

DWORD GetTotalProcessorCount() {
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo); // 获取系统信息
    return sysInfo.dwNumberOfProcessors; // 返回处理器数量
}

std::vector<DWORD_PTR> getProcessAffinityMask(const std::string& filename) {
    std::ifstream file(filename);
    std::string line;
    std::vector<DWORD_PTR> affinityMasks;
    DWORD_PTR currentMask = 0;
    int coreCount = 0; // 当前处理核心计数

    DWORD totalcore = GetTotalProcessorCount();
    static bool firsttime = true;
    if(firsttime){
        std::cout<< "[\033[32mINFO\033[0m]Cores: "<<totalcore<<endl;
        firsttime = false;
    }
    DWORD totalcore_c = 0;

    if (!file.is_open()) {
        std::cerr << "Unable to open file: " << filename << std::endl;
        return affinityMasks; // 返回空向量
    }

    // 读取文件内容
    while (std::getline(file, line)) {
        for (char c : line) {
            if (c == '#' || c == '=') {
                if (c == '#' && coreCount < 64) { // 只有当核心计数小于64时，#才会被考虑
                    currentMask |= (1ULL << coreCount);
                }
                ++coreCount;
                ++totalcore_c;

                if (coreCount == 64) { // 当达到64个核心，转移到下一个处理器组
                    affinityMasks.push_back(currentMask);
                    currentMask = 0; // 重置当前掩码
                    coreCount = 0; // 重置核心计数
                }
                if(totalcore_c == totalcore){
                    break;
                }
            }
        }
    }

    if (coreCount > 0) { // 确保最后一个掩码被添加，即使它不完整
        affinityMasks.push_back(currentMask);
    }

    return affinityMasks;
}

int main(int argc, char* argv[]) {

    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <config_file> <program> [args...]" << std::endl;
        return 1;
    }

    std::string configPath = argv[1];
    std::ifstream configFile(configPath);
    if (!configFile.is_open()) {
        std::cerr << "Failed to open config file: " << configPath << std::endl;
        return 1;
    }
    configFile.close();
    auto processAffinityMaskV = getProcessAffinityMask(configPath);

    // 创建进程
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    std::string cmdLine;
    for (int i = 2; i < argc; ++i) {
        cmdLine += std::string(argv[i]) + " ";
    }

    if (!CreateProcessA(NULL, &cmdLine[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        std::cerr << "CreateProcess failed: " << GetLastError() << std::endl;
        return 1;
    }

    // DWORD_PTR processAffinityMask = processAffinityMaskV[0];

    // 设置进程亲和性
    bool firsttime = true;
    Sleep(50);
    while(1){
        // if (SetProcessAffinityMask(pi.hProcess, processAffinityMask)) {
        if (SetThreadAffinityPerGroup(processAffinityMaskV,pi)){
            std::cerr << "SetProcessAffinityMask failed: " << GetLastError() << std::endl;
            //杀死被运行的进程
            TerminateProcess(pi.hProcess, 1);
            return 1;
        }else{
            //显示：[(绿色)OK]运行"...”命令在“...”文件上（英文）
            if(firsttime){
                std::cout << "[\033[32mOK\033[0m] Running \"" << cmdLine << "\" on \"" << configPath << "\"" << std::endl;
                firsttime = false;  
            }
        }
        //如果进程结束，退出循环
        DWORD exitCode;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        if (exitCode != STILL_ACTIVE) {
            break;
        }
        Sleep(100);
    }
    // 等待进程结束
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);


    // 等待进程结束
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return 0;
}
