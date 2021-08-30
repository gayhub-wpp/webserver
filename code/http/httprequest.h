#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>
#include <errno.h>     
#include <mysql/mysql.h>  //mysql

#include "../buffer/buffer.h"
#include "../log/log.h"
#include "../pool/sqlconnpool.h"
#include "../pool/sqlconnRAII.h"

class HttpRequest {
public:
    enum PARSE_STATE {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH,        
    };

    enum HTTP_CODE {
        NO_REQUEST = 0,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURSE,
        FORBIDDENT_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION,
    };
    
    HttpRequest() { Init(); }
    ~HttpRequest() = default;

    void Init();
    bool parse(Buffer& buff);    //解析HTTP请求

    std::string path() const;
    std::string& path();
    std::string method() const;
    std::string version() const;
    std::string GetPost(const std::string& key) const;    //在post方式下获取信息的接口
    std::string GetPost(const char* key) const;

    bool IsKeepAlive() const;     //获取HTTP连接是否KeepAlive

    /* 
    todo 
    void HttpConn::ParseFormData() {}
    void HttpConn::ParseJson() {}
    */

private:
    bool ParseRequestLine_(const std::string& line); // 解析请求行
    void ParseHeader_(const std::string& line);      // 解析请求头
    void ParseBody_(const std::string& line);        // 解析数据体

    void ParsePath_();            //解析出路径信息
    void ParsePost_();            //解析post报文
    void ParseFromUrlencoded_();

    static bool UserVerify(const std::string& name, const std::string& pwd, bool isLogin);

    PARSE_STATE state_;    //实现自动机的state变量
    std::string method_, path_, version_, body_;   //HTTP方式、路径、版本和数据体
    std::unordered_map<std::string, std::string> header_;  //请求头已经解析出来的信息。
    std::unordered_map<std::string, std::string> post_;    //post已经解析出来的信息

    static const std::unordered_set<std::string> DEFAULT_HTML;   //默认的网页名称
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;
    static int ConverHex(char ch);    //转换Hex格式
};


#endif //HTTP_REQUEST_H