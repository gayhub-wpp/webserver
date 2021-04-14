#include <arpa/inet.h>
#include <iostream>
using namespace std;

int main(int argc, char const *argv[])
{
    
    const char* s = "1.2.3.4";
    // cout << inet_addr(s) << endl;   //点分十进制->网络字节序

    in_addr* in;
    // int a = inet_aton(s, in);
    // cout << a  << " " << in->s_addr << endl;     //点分十进制->网络字节序
    u_int32_t* v;
    int a = inet_pton(AF_INET, s, in);
    cout << a  << " " << in->s_addr << endl;     //点分十进制->网络字节序

    // in_addr in;
    
    // in.s_addr = 67305985;
    // cout << inet_ntoa(in) << endl;     //网络字节序——>点分十进制


    return 0;
}

