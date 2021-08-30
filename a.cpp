#include<iostream>
using namespace std;
int handle(int a,int b){
    if(a ==0) return b;
    if(b ==0) return a;
    int i = (a^b);
    int j = (a&b)<<1;
    return handle(i,j);
    }


int main(){
       
    // cout << handle(256,64) << endl;    
    // int a[5] = {1,2,3,4,5};
    // cout <<*(*(&a+1)-1) <<endl;
    char a[4];
    char *b[5];
    cout << sizeof(a) <<" " <<sizeof(b) <<endl;

    return 0;
}