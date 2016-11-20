#include<iostream>
#include <string>
using namespace std;
int main()
{
  char buf[10] = "hello";
  string temp(buf);
  cout<<temp<<temp.size();

}
