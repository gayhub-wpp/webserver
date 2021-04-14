#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#define BUFFER 4096 //读缓冲

//主状态机
enum CHECK_STATE{
    CHECK_STATE_REQUESTLINE = 0, //分析请求行
    CHECK_STATE_HEADER           //分析头部字段
};

//从状态机
enum LINE_STATE{
    LINE_OK = 0,   //读取到一个完整的行
    LINE_BAD,      //行出错
    LINE_OPEN      //行数据尚且不完整
};
// 服务器处理http请求的结果
enum HTTP_CODE{
    NO_REQUEST, GET_REQUEST, BAD_REQUEST, FORBIDDEN_REQUEST, INTERNAL_ERROR, CLOSED_CONNECTION
};

static const char* szret[] = {"I get a correct result\n", "something wrong\n"};

// 从状态机，解析一行内容
LINE_STATE parse_line(char * buffer, int & checked_index, int & read_index){
    char temp;
    //checked_index 指向buffer中正在分析的字节， read_index指向buffer中客户数据的尾部的下一字节。0-checked分析完毕
    for (; checked_index < read_index; ++checked_index)
    {
        temp = buffer[checked_index];
        if (temp == 'r')
        {
            if ((checked_index +1) == read_index)
            {
                return LINE_OPEN;
            }
            else if (buffer[checked_index+1] == '\n')
            {
                buffer[checked_index++] = '\0';
                buffer[checked_index++] = '\0';
                return LINE_OK;
            }
            else{
                return LINE_BAD;
            }
        }
        else if (temp == '\n')
        {
            if ((checked_index > 1) && buffer[checked_index-1] == '\r')
            {
                buffer[checked_index++] = '\0';
                buffer[checked_index++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

HTTP_CODE parse_requestline(char* temp, CHECK_STATE& checkstate){
    // 如果要查找多个字符，需要使用 strpbrk 函数。
    //char *strpbrk(const char *s1,const char *s2);该函数在源字符串（s1）中按从前到后顺序找出最先含有搜索字符串（s2）中任一字符的位置并返回，
    char* url = strpbrk(temp, "\t");  

    if (!url)
    {
        return BAD_REQUEST;
    }
    *url++ = '\0';

    char* method = temp;
    if (strcasecmp(method, "GET") == 0) //strcasecmp（忽略大小写比较字符串）仅支持GET方法
    {
        printf("The request method is GET\n");
    }
    else{
        return BAD_REQUEST;
    }    

    url += strspn(url, "\t");  //strspn返回字符串url开头连续包含"\t"的字符数目。
    char
    
}
