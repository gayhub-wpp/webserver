## 基础API
### fputs
```cpp
#include <stdio.h>
int fputs(const char *str, FILE *stream);
```
- str，一个数组，包含了要写入的以空字符终止的字符序列。
- stream，指向FILE对象的指针，该FILE对象标识了要被写入字符串的流。

### 可变参数宏__VA_ARGS__
__VA_ARGS__是一个可变参数的宏，定义时宏定义中参数列表的最后一个参数为省略号，在实际使用时会发现有时会加##，有时又不加。
```cpp
//最简单的定义
#define my_print1(...)  printf(__VA_ARGS__)

//搭配va_list的format使用
#define my_print2(format, ...) printf(format, __VA_ARGS__)  
#define my_print3(format, ...) printf(format, ##__VA_ARGS__)
```
__VA_ARGS__宏前面加上##的作用在于，当可变参数的个数为0时，这里printf参数列表中的的##会把前面多余的","去掉，否则会编译出错，建议使用后面这种，使得程序更加健壮。




### fflush
```cpp
#include <stdio.h>
int fflush(FILE *stream);
```
fflush()会强迫将缓冲区内的数据写回参数stream 指定的文件中，如果参数stream 为NULL，fflush()会将所有打开的文件数据更新。

在使用多个输出函数连续进行多次输出到控制台时，有可能下一个数据再上一个数据还没输出完毕，还在输出缓冲区中时，下一个printf就把另一个数据加入输出缓冲区，结果冲掉了原来的数据，出现输出错误。

在prinf()后加上fflush(stdout); 强制马上输出到控制台，可以避免出现上述错误。


## stdarg
stdarg.h是C语言中C标准库的头文件，stdarg是由standard（标准） arguments（参数）简化而来，主要目的为让函数能够接收不定量参数。[1] C++的cstdarg头文件中也提供这样的机能；虽然与C的头文件是兼容的，但是也有冲突存在。

不定参数函数（Variadic functions）是stdarg.h内容典型的应用


### stdarg.h数据类型
|名称	|描述	|兼容|
|------|-----|-------|
|va_list|	用来保存宏va_arg与宏va_end所需信息|	C89|

### stdarg.h宏

|名称|	描述|	兼容|
|------|-----|-------|
|va_start|	使va_list指向起始的参数|	C89|
|va_arg	|检索参数	|C89|
|va_end	|释放va_list	|C89|
|va_copy|	拷贝va_list的内容|	C99|

以一个例子进行说明
(1) 当出现无法列出所有传递参数数目和类型的时候，用省略号指定参数列表

void PrintInt (int cnt, ...)
(2) 使用 va_list 获取参数并进行处理
- 定义一个 va_list 类型的变量，va_list 类型变量定义为 ap
- 调用 va_start，对 ap 进行初始化，让它指向可变参数表里面的第一个参数
- 使用 va_arg 获取参数，并使用参数
- 获取所有的参数之后，将 ap 指针关掉
```cpp
void PrintInt(int cnt, ...)  { 
    va_list ap;
    int value;

    va_start(ap, cnt);

    for (int i = 0; i < cnt; i++) {
        value = va_arg(ap, int);
        printf("%d\n", value); 
    }

    va_end(ap);
}
```
### 访问参数
访问未命名的参数，首先必须在不定参数函数中声明va_list数据类型的变量。调用va_start并传入两个参数：第一个参数为va_list数据类型的变量，第二个参数为函数的动态参数前面最后一个已命名的参数名称，接着每一调用va_arg就会返回下一个参数，va_arg的第一个参数为va_list，第二个参数为返回的数据类型。最后va_end必须在函数返回前被va_list调用(当作参数)。(没有要求要读取完所有参数)

C99提供额外的宏，va_copy，它能够复制va_list。而va_copy(va2, va1)意思为拷贝va1到va2。

没有机制定义该怎么判别传递到函数的参数量或者数据类型。函数通常需要知道或确定它们变化的方法。共通的惯例包含:

- 使用printf或scanf类的格式化字符串来嵌入明确指定的数据类型。
- 在不定参数最后的标记值(sentinel value)。
- 总数变量来指明不定参数的数量。

## snprintf
C 库函数 int snprintf(char *str, size_t size, const char *format, ...) 设将可变参数(...)按照 format 格式化成字符串，并将字符串复制到 str 中，size 为要写入的字符的最大数目，超过 size 会被截断。



