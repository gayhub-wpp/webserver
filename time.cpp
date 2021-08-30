#include <ctime>
#include <iostream>

using namespace std;

int main(int argc, char const *argv[])
{
    time_t now = time(0);
    cout <<now<<endl;
    tm* ltm = localtime(&now);
    cout <<ltm->tm_year+1900 << " " <<ltm->tm_mon+1 << " "<<ltm->tm_mday <<endl;
    return 0;
}
