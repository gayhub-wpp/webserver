#include <iostream>
#include <stdio.h>
#include <string.h>

using namespace std;

int main(int argc, char const *argv[])
{
    int count;
    printf("The command line has %d arguments :\n",argc-1);
    for (count = 1; count < argc; ++count) {
        printf("%d: %s\n",count,argv[count]);
    }

    return 0;
}