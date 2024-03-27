# Windows-SetProcessGroupAffinity
To Set Process Group Affinity for win11, as there are only GetProcessGroupAffinity-SetThreadGroupAffinity-GetThreadGroupAffinity-SetProcessAffinityMask, but no SetProcessGroupAffinity.
Refer to cores.txt for usage.

要为 win11 设置进程组亲和性，也就是给进程绑核
因为只有 GetProcessGroupAffinity-SetThreadGroupAffinity-GetThreadGroupAffinity-SetProcessAffinityMask 而没有 SetProcessGroupAffinity，所以补充了新的函数作为WindowsAPI的补充
使用方法参考cores.txt
