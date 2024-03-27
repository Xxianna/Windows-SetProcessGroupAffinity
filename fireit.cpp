#include <iostream>
#include <vector>
#include <thread>
#include <Windows.h>

// 线程函数，执行无限循环的加法操作
void threadFunction(int threadNum) {
    // 绑定当前线程到指定的CPU核心
    DWORD_PTR affinityMask = 1ULL << threadNum;
    SetThreadAffinityMask(GetCurrentThread(), affinityMask);

    // 无限循环的加法操作，用于CPU负载
    volatile long long counter = 0;
    while (true) {
        ++counter;
    }
}

int main() {
    // 获取系统的CPU核心数量
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    int numCores = sysInfo.dwNumberOfProcessors;

    // 为每个核心创建一个线程
    std::vector<std::thread> threads;
    for (int i = 0; i < numCores; ++i) {
        threads.push_back(std::thread(threadFunction, i));
    }

    // 等待所有线程结束（实际上线程会无限循环，不会结束）
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    return 0;
}
