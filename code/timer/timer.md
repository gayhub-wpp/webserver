为了实现定时器的功能，我们首先需要辨别每一个HTTP连接，每一个HTTP连接会有一个独有的描述符（socket），我们可以用这个描述符来标记这个连接，记为id。同时，我们还需要设置每一个HTTP连接的过期时间。
## HeapTimer()
HeapTimer(){heap_.reserve(64);} 
预先分配64大小

## ~HeapTimer()
~HeapTimer(){clear();}

## adjust 
void adjust(int id, int newExpires);
调整指定id的结点

## add
void add(int id, int timeOut, const TimeoutCallBack& cb);   
添加一个定时器

## doWork
void doWork(int id);   
删除制定id节点，并且用指针触发处理函数

## clear
void clear();

## tick
void tick();       
清除超时结点 

## pop
void pop();

## GetNextTick
int GetNextTick();   
下一次处理过期定时器的时间

## del_
void del_(size_t i);                     
删除指定位置的结点

## siftup_
void siftup_(size_t i);                  
向上调整

## siftdown_
bool siftdown_(size_t index, size_t n);  
将index位置的节点向下调整

## SwapNode_
void SwapNode_(size_t i, size_t j);      
交换两个结点位置

## heap_
std::vector<TimerNode> heap_;    

## ref_
std::unordered_map<int, size_t> ref_;    
映射一个fd对应的定时器在heap_中的位置