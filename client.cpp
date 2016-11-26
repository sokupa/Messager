#include <algorithm>
#include <arpa/inet.h>
#include <arpa/inet.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <list>
#include <netdb.h>
#include <netinet/in.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h> //for mkdir
#include <sys/types.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <vector>
#define DEBUG_LOG false
#define BUFSIZE 1024
using std::endl;
using std::cout;
using std::cin;
using std::string;
using std::unordered_map;
using std::thread;
using std::vector;
using std::stringstream;
using std::to_string;

class chatclient
{
private:
    int m_sockfd;              // server port
    struct sockaddr_in m_addr; // server address
    int m_id;                  // id received by server
    string m_name;
    static unordered_map<string, int> cmdmap;

public:
    chatclient()
        : m_sockfd(0)
    {
        memset(&m_addr, 0, sizeof(m_addr));
    }
    bool create();
    bool bind(int port);
    bool connect(const string&, const int);
    int sendto(bool isfile, string out);
    int sendto(bool, int);
    int sendto(int, bool, int);
    int sendto(bool isfile, char (&buf)[BUFSIZE], string msg);
    int sendto(int sock, bool isfile, char (&buf)[BUFSIZE], string const out);
    int recvfrom(int&);
    int recvfrom(string&);
    int recvfrom(char (&buf)[BUFSIZE], int len);
    void close();
    bool sendfile(string);
    int recvfile();
    int getfilesize(std::ifstream&);
    string getpath();
    void setname();
    bool createdir(string);
    bool validatecmd(int&, string);
    // vector<string> parse(string);
    string getname();
    static int handlerecv(chatclient);
    static void handlesend(chatclient);
    void printlog(string);
};

unordered_map<string, int> chatclient::cmdmap = { { "BC", 0 }, { "BC-FILE", 1 }, { "UC", 2 }, { "UC-FILE", 3 },
    { "BLC", 4 }, { "BLC-FILE", 5 }, { "$NOK$", 6 } };

void chatclient::printlog(string str)
{
    if(DEBUG_LOG)
        cout << str << endl;
}

bool chatclient::create()
{
    m_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(m_sockfd == -1)
        return false;
}

bool chatclient::bind(int port)
{
    if(m_sockfd == -1)
        return false;
    m_addr.sin_family = AF_INET;
    m_addr.sin_addr.s_addr = INADDR_ANY;
    m_addr.sin_port = htons(port);
    return ::bind(m_sockfd, (struct sockaddr*)&m_addr, sizeof(m_addr)) == -1 ? false : true;
}

bool chatclient::connect(const string& servadd, const int servport)
{
    if(m_sockfd == -1)
        return false;
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = htons(servport);
    inet_pton(AF_INET, servadd.c_str(), &m_addr.sin_addr);
    int retval = ::connect(m_sockfd, (sockaddr*)&m_addr, sizeof(m_addr));
    return retval == 0 ? true : false;
}

int chatclient::sendto(bool isfile, string const out)
{
    int status = 0;
    printlog("send to before ");
    do {
        status = ::send(m_sockfd, out.c_str(), out.size(), MSG_NOSIGNAL);
    } while(status <= 0);
    printlog("after send");
    return status;
}

int chatclient::sendto(bool isfile, int const out)
{
    int status = 0;
    do {
        status = ::send(m_sockfd, &out, sizeof(int), MSG_NOSIGNAL);
    } while(status <= 0);
    return status;
}

int chatclient::sendto(int sock, bool isfile, int const out)
{
    int status = 0;
    do {
        status = ::send(sock, &out, sizeof(int), MSG_NOSIGNAL);
    } while(status <= 0);
    return status;
}

int chatclient::sendto(bool isfile, char (&buf)[BUFSIZE], string msg)
{
    int len = msg.size();
    printlog("buf send before " + msg);
    sendto(false, len); // send buf len
    int status = 0;
    strcpy(buf, msg.c_str());
    do {
        status = ::send(m_sockfd, buf, len, 0);
    } while(status <= 0);
    printlog("buf send after "+to_string(status));
    return status;
}

int chatclient::sendto(int sock, bool isfile, char (&buf)[BUFSIZE], string const msg)
{
    int len = msg.size();
    int status = 0;
    sendto(sock, false, len); // send buf len
    strcpy(buf, msg.c_str());
    do {
        status = ::send(sock, buf, len, 0);
    } while(status <= 0);
    return status;
}
int chatclient::recvfrom(string& inp)
{
    char cstr[100];
    int size = 0;
    memset(cstr, 0, 100);
    printlog("recv from before ");
    do {
        size = ::recv(m_sockfd, cstr, 100, 0);
    } while(size <= 0);
    inp = std::string(cstr, size);
    printlog("recv from after " + to_string(size));
    return size;
}

int chatclient::recvfrom(char (&buf)[BUFSIZE], int len)
{
    int status = 0;
    bzero(buf, BUFSIZE);
    do {
        status = ::recv(m_sockfd, buf, len, 0);
    } while(status <= 0);
    return status;
}

int chatclient::recvfrom(int& inp)
{
    int status = 0;
    int val = 0;
    do {
        status = ::recv(m_sockfd, &val, sizeof(int), 0);
    } while(status <= 0);
    inp = val;
    return status;
}
void chatclient::close()
{
    if(m_sockfd == -1)
        return;
    cout << "socket close" << endl;
    ::close(m_sockfd);
}

string chatclient::getpath()
{
    char buf[100];
    int len = readlink("/proc/self/exe", buf, 100);
    return string(buf, (len > 0 ? len : 0));
}
bool chatclient::createdir(string path)
{
    bool success = false;
    printlog("creating dir " + path);
    int created = ::mkdir(path.c_str(), 0775);
    if(created == -1) { // create failed check parent
        switch(errno) { // global in errno.h
        case ENOENT: {
            string parent = path.substr(0, path.find_last_of('/'));
            if(::mkdir(parent.c_str(), 0775))
                created = ::mkdir(path.c_str(), 0775);
            success = (created == 0);
            break;
        }
        case EEXIST:
            success = true;
            break;
        default:
            success = false;
            break;
        }
    } else {
        success = true;
    }
    return success;
}

int chatclient::getfilesize(std::ifstream& file)
{
    file.ignore(std::numeric_limits<std::streamsize>::max());
    int size = file.gcount();
    file.clear(); //  Since ignore will have set eof.
    file.seekg(0, std::ios_base::beg);
    return size;
}

bool chatclient::sendfile(string filename)
{
    string savedir = getpath(); // return path of current exe running
    savedir = savedir + "_" + std::to_string(m_id);
    createdir(savedir);
    savedir = savedir + "/" + filename;
    printlog("path " + savedir);
    char buf[BUFSIZE];
    int status = 0;
    std::ifstream in(savedir, std::ios::in | std::ios::binary);
    if(!in) {
        cout << "error in opening file" << endl;
        sendto(false, buf, "Invalid file");
        return false;
    }
    int size = getfilesize(in);
    printlog("filename " + filename);

    int len = filename.size();

    // send filename size
    status = sendto(false, buf, filename);
    // send filename
    // status = sendto(false, filename); //;sendto(false, filename);
    printlog("file size " + to_string(size));
    string filesz = std::to_string(size);
    // send len
    len = filesz.size();
    status = sendto(false, buf, filesz);
    // send fille size
    // status = sendto(false, filesz);
    printlog("sendfing file ");
    long sentbytes = 0;
    long read;
    bzero(buf, BUFSIZE);
    while((read = in.read(buf, BUFSIZE).gcount()) > 0) {
        if(::send(m_sockfd, buf, read, 0) < 0) {
            return 1;
        }
        printlog(" sent size ");
        bzero(buf, BUFSIZE);
    }
    printlog("send completed");
    return true;
}

int chatclient::recvfile()
{
    int size = 0;
    string savedir = getpath(); // path of client.exe i.e ~/client
    // create dir for client
    savedir = savedir + "_" + std::to_string(m_id); //~/client_1
    if(!createdir(savedir)) {
        cout << "Error in creating dir" << endl;
        return -1;
    }
    // wait for filename from server
    string filename;
    printlog("waiting to recieve filename ");
    int len = 0;
    char buf[BUFSIZE];
    bzero(buf, BUFSIZE);
    int status = 0;
    status = recvfrom(len);
    recvfrom(buf, len);

    filename = std::string(buf, len);
    printlog(" Recieved file with name " + filename);
    printlog("waiting for file size");
    string val;
    status = recvfrom(len);
    printlog(" len " + to_string(len));
    bzero(buf, BUFSIZE);
    recvfrom(buf, len); // recieve filesz
    val = std::string(buf, len);
    size = std::stoi(val);
    size = std::stoi(val);
    printlog(" Recieved file with size " + to_string(size));
    filename = savedir + "/" + filename;
    std::ofstream out(filename, std::ios::out | std::ios::binary);
    if(!out.is_open()) {
        cout << "fail to open " << endl;
        return -1;
    }
    int total = 0;
    printlog("Writing to file ");
    int readsz = 0;

    while(total < size) {
        readsz = ::recv(m_sockfd, buf, BUFSIZE, 0); //;recvfrom(temp);
        if(readsz < 0) {
            cout << "read error" << endl;
            return -1;
        }
        out.write(buf, readsz); //;out.write(temp.c_str(), temp.size());
        total += readsz;
        memset(buf, 0, readsz);
        if(readsz == 0)
            break;
    }
    printlog("read finished ");
    out.close();
    return total;
}
bool chatclient::validatecmd(int& cmdid, string command)
{
    auto it = cmdmap.find(command);
    if(it != cmdmap.end()) {
        cmdid = it->second;
        return true;
    }
    return false;
}

string chatclient::getname()
{
    return m_name;
}

void chatclient::handlesend(chatclient clientsock)
{
    string input;
    string command;
    while(1) {
        cout << "enter command" << endl;
        getline(std::cin, input);
        clientsock.printlog("entered command" + command);
        int cmdid = 0;
        command = input.substr(0, input.find_first_of(" ")); //;args[0];
        input = input.substr(input.find_first_of(" ") + 1);

        if(clientsock.validatecmd(cmdid, command) == false) {
            cout << " Invalid command line argument" << endl;
            cout << "--------commands------------" << endl;
            cout << " BC      ->   Broadcast" << endl;
            cout << " BC-FILE      ->   Broadcast File" << endl;
            cout << " UC      ->   Unicaset" << endl;
            cout << " UC-FILE       ->   Unicaset File" << endl;
            cout << " BLC      ->   Blockcast " << endl;
            cout << " BLC-FILE      ->   Blockcast FILe " << endl;
            continue;
        }
        // send command to server
        int status = 0;
        char buf[BUFSIZE];
        // send len
        status = clientsock.sendto(false, buf, command);
        switch(cmdid) {
        // message Broadcast
        case 0: {
            while(input.empty()) {
                cout << " enter message to broadcast" << endl;
                getline(std::cin, input);
            }
            // send the message
            status = clientsock.sendto(false, buf, input);
        } break;
        // unicast
        case 2:
        case 4: {
            string msg = input.substr(0, input.find_last_of(" "));
            string to = input.substr(input.find_last_of(" ") + 1);
            clientsock.printlog(" msg " + msg + " to " + to);
            while(msg.empty()) {
                cout << " enter message " << endl;
                getline(std::cin, msg);
            }
            while(to == clientsock.getname() || to.size() == 0) {
                cout << " trying to send message to self or none" << endl;
                cout << " enter client as client_<id>" << endl;
                getline(std::cin, to);
            }
            msg = msg + "###" + to; // message###client
            // send the message
            status = clientsock.sendto(false, buf, msg);
        } break;
        case 1:
            // broadcast file
            while(input.empty()) {
                cout << " enter filename" << endl;
                getline(std::cin, input);
            }
            if(!clientsock.sendfile(input)) {
                cout << " retry with valid file " << endl;
                continue;
            }
            break;
        case 3:
        case 5: {
            // cout<<"case 3 "<<input<<endl;
            string filename = input.substr(0, input.find_last_of(" "));
            string to = input.substr(input.find_last_of(" ") + 1);
            while(filename.empty()) {
                cout << " enter filename" << endl;
                getline(std::cin, input);
            }
            // send id to send or block;
            status = clientsock.sendto(false, buf, to);
            if(!clientsock.sendfile(filename)) {
                cout << " retry with valid file " << endl;
                continue;
            }
        } break;

        case 6:
            break;
        }
    }
}

int chatclient::handlerecv(chatclient clientsock)
{
    int len = 0, status = 0;
    char buf[BUFSIZE];
    // command
    string cmd;
    // get command len
    while(true) {
        clientsock.recvfrom(len);

        // get command
        status = clientsock.recvfrom(buf, len);
        cmd = string(buf, len);
        int cmdid;
        if(clientsock.validatecmd(cmdid, cmd) == false) {
            cout << " Invalid command from server";
            continue;
        }
        switch(cmdid) {
        // recieve msg
        case 0:
        case 2:
        case 4: {
            string msg;
            clientsock.recvfrom(len);
            clientsock.recvfrom(buf, len);
            msg = string(buf, len);
            cout << msg << endl;
        } break;
        // recieve file
        case 1:
        case 3:
        case 5:
            clientsock.recvfile();
            break;
        case 6: {
            string msg;
            cout << " wrong value sent to server" << endl;
            clientsock.recvfrom(len);
            clientsock.recvfrom(buf, len);
            msg = string(buf, len);
            cout << msg << endl;
        } break;
        default:
            break;
        }
    }
}
void chatclient::setname()
{
    // ask server to get name
    int id = 0;
    recvfrom(id);
    m_id = id;
    m_name = string("client_" + std::to_string(id));
    cout << "my name " << m_name << endl;
}
int main(int argc, char** argv)
{
    chatclient clientsock;
    string hostname = "127.0.0.1";
    int port = 61823;
    clientsock.create();
    clientsock.connect(hostname, port);
    clientsock.setname();
    thread send(chatclient::handlesend, (clientsock));
    thread receiver(chatclient::handlerecv, (clientsock));
    send.join();
    receiver.join();
    clientsock.close();
    return 0;
}
