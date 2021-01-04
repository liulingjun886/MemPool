# MemPool
高并发内存池

ThreadCache 类 每个线程创建一独有实例，运行在该线程的程序通过其获得内存

CentralCache 类 单例类，分发和回收 ThreadCache 实例的内存，用于多线程间内存分平衡

PageCache 类 单例类， 向操作系统申请内存块，并供CentralCache类实例切割使用，合并小内存。

该项目在centos6.10编译通过，可正常运行。
因项目测试使用单线程，故不加锁。
如所涉及多线程开发需在 CentralCache 和 PageCache 类中加锁，加锁位置源文件中亦有标识。
