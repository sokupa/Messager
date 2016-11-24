#include <iostream>
#include <cstring>
#define SIZE 100
using namespace std;
void edit( char (&buf)[SIZE])
{
memcpy(buf,"hello",5);
}
int main()
{
char buf[SIZE];
edit(buf);
cout<<buf;
}
