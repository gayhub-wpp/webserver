#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <unordered_map>
#include <fcntl.h>       // open
#include <unistd.h>      // close
#include <sys/stat.h>    // stat
#include <sys/mman.h>    // mmap, munmap

#include "../buffer/buffer.h"
#include "../log/log.h"

class HttpResponse {
public:
    HttpResponse();
    ~HttpResponse();

    void Init(const std::string& srcDir, std::string& path, bool isKeepAlive = false, int code = -1);
    void MakeResponse(Buffer& buff);     //生成响应报文的主函数
    void UnmapFile();                    //共享内存的扫尾函数
    char* File();                        //返回文件信息
    size_t FileLen() const;              //返回文件信息
    void ErrorContent(Buffer& buff, std::string message);   //如果所请求的文件打不开，我们需要返回相应的错误信息
    int Code() const { return code_; }   //返回状态码

private:
    void AddStateLine_(Buffer &buff);   //生成请求行，请求头和数据体
    void AddHeader_(Buffer &buff);
    void AddContent_(Buffer &buff);

    void ErrorHtml_();    //对于4XX的状态码是分开考虑的
    std::string GetFileType_();   //文件类型信息

    int code_;            //HTTP的状态
    bool isKeepAlive_;    //HTTP连接是否处于KeepAlive状态

    std::string path_;    //解析得到的路径
    std::string srcDir_;  //根目录
    
    char* mmFile_;        //共享内存
    struct stat mmFileStat_;

    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;  //后缀名到文件类型的映射关系
    static const std::unordered_map<int, std::string> CODE_STATUS;    //状态码到相应状态(字符串类型)的映射。 
    static const std::unordered_map<int, std::string> CODE_PATH;
};


#endif //HTTP_RESPONSE_H