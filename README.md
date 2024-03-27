# Windows-SetProcessGroupAffinity
To Set Process Group Affinity for win11, as there are only GetProcessGroupAffinity-SetThreadGroupAffinity-GetThreadGroupAffinity-SetProcessAffinityMask, but no SetProcessGroupAffinity.</br>
Refer to cores.txt for usage.
</br></br>
要为 win11 设置进程组亲和性，也就是给进程绑核</br>
因为只有 GetProcessGroupAffinity-SetThreadGroupAffinity-GetThreadGroupAffinity-SetProcessAffinityMask 而没有 SetProcessGroupAffinity，所以补充了新的函数作为WindowsAPI的补充</br>
使用方法参考cores.txt
</br></br>

目前的cores.txt里是8核处理器关闭超线程运行程序的演示
</br>
Currently in cores.txt is a demo of an 8-core processor running a program with hyperthreading turned off
