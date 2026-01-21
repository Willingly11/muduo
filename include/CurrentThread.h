#pragma once

namespace CurrentThread
{
    extern thread_local int t_cachedTid; // 保存tid缓存 因为系统调用非常耗时 拿到tid后将其保存

    void cacheTid();

    inline int tid() // 内联函数只在当前文件中起作用
    {
        // 
        if (__builtin_expect(t_cachedTid == 0, 0)) 
        {
            cacheTid();
        }
        return t_cachedTid;
    }
}

/**
extern thread_local int t_cachedTid;
- extern: 
声明这个变量（或函数）存在，但内存分配在别的 `.cpp` 文件里。这解决了头文件被多次包含导致的“重定义”问题。

- 编译与链接
1. 编译 (Compile)：编译器分别把 main.cpp 编译成 main.o，把 CurrentThread.cpp 编译成 CurrentThread.o。此时它们是独立的“半成品”。
2. 链接 (Link)：链接器 (Linker) 负责把这一堆 .o 文件像拼积木一样拼在一起，最后生成唯一的一个可执行文件（比如 myserver）。

- 只在头文件中声明，在其他地方（.cpp 文件）定义，确保只有一份实例存在，避免重定义问题
若定义在头文件里里，假设 A.cpp 引用了头文件，B.cpp 也引用了头文件。
编译器在编译 A 和 B 时，会分别为它们生成一个 t_cachedTid 符号。
当链接器（Linker）试图把 A 和 B 合并成一个可执行文件时，
会报错：multiple definition of 't_cachedTid'（符号重定义）
 */

/**
- 头文件中 inline int tid()
一是为了性能，这种想被内联的高频小函数，必须写在头文件里；
二是为了避免链接错误，发生重定义的问题
- __builtin_expect 的作用
    1. __builtin_expect 是一种底层优化 此语句意思是如果还未获取tid 进入if 通过cacheTid()系统调用获取tid
    2. __builtin_expect(..., 0) 
        含义：这是 GCC/Clang 的底层优化指令,表示该条件不太可能成立（即大部分情况下tid已经被缓存了）
        效果：CPU 会提前预加载 else 分支（直接返回数据）的指令流水线，从而减少分支预测失败带来的性能损失
 */