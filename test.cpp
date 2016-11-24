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

vector<string> chatclient::parse(string command)
{
    std::stringstream inp = stringstream(command);
    vector<string> res;
    string temp;
    // delimit based on space
    while(std::getline(inp, temp, ' ')) {
        res.push_back(temp);
    }
    return res;
}
