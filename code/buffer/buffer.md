## 成员变量
vector<int> buffer 缓冲区
readPos_           读位置
writePos_          写位置

## 成员函数
### ReadableBytes()
可读长度
### WritableBytes()
可写长度
### PrependableBytes()
返回读指针在缓冲区的位置
### Peek()
缓冲区起始地址+读位置，可读地址
### Retrieve(size_t len)
读len长的内容，读指针更新
### RetrieveUntil(const char* end) 
将读指针移到指定位置
### RetrieveAll()
清空缓冲区
### RetrieveAllToStr()
缓冲区所有内容所有变成字符串，再清空
### BeginWrite()
缓冲区起始地址+写位置，可写起始地址
### HasWritten(size_t len)
写入len长度的内容，更新写指针
### Append()
将str加入buffer   ( str.data()生成一个const char*指针，指向一个临时数组。)
### EnsureWriteable(size_t len)
确保len长的内容可写，空间不够给buffer增加len的空间
### ReadFd(int fd, int* saveErrno)
读文件描述符上的数据到buffer，更新写指针
### WriteFd(int fd, int* saveErrno)
将buffer数据写到fd上，更新读指针
### MakeSpace_(size_t len)
若剩余空间 小于 len, 增加len空间
若足够，将缓冲区数据copy到缓冲区begin的地方，空出连续的大于len的空间