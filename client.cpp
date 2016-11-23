#include <algorithm>
#include <arpa/inet.h>
#include <arpa/inet.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <list>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h> //for mkdir
#include <sys/types.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <unordered_set>
#define BUFSIZE 1024
using std::endl;
using std::cout;
using std::cin;
using std::string;

class chatclient
{
private:
    int m_sockfd;              // server port
    struct sockaddr_in m_addr; // server address
    string m_name;

public:
    chatclient()
        : m_sockfd(0)
    {
        memset(&m_addr, 0, sizeof(m_addr));
    }
    bool create();
    bool bind(int port);
    bool connect(const string&, const int);
    bool sendto(bool isfile, string out); // todo
    int recvfrom(string&);                // to do
    void close();
    bool sendfile();
    int recvfile();
    int getfilesize(std::ifstream&);
    string getpath();
    string setmyname(string);
    bool createdir(string);
};

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

bool chatclient::sendto(bool isfile, string out)
{
    return send(m_sockfd, out.c_str(), out.size(), MSG_NOSIGNAL) == -1 ? false : true;
}

int chatclient::recvfrom(string& inp)
{
    char cstr[100];
    memset(cstr, 0, 100);
    int size = recv(m_sockfd, cstr, 100, 0);
    if(size == -1)
        return 0;
    inp = std::string(cstr, size);
    return size;
}

void chatclient::close()
{
    if(m_sockfd == -1)
        return;
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
    cout << "createdir " << path << endl;
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
    cout << success << endl;
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

bool chatclient::sendfile()
{
    string savedir = getpath(); // return path of current exe running
    savedir = savedir + "_dir";
    createdir(savedir);
    cout << savedir << endl;
    cout << "enter file name" << endl;
    string filename;
    cin >> filename;
    savedir = savedir + "/" + filename;
    std::ifstream in(savedir, std::ios::in | std::ios::binary);
    if(!in) {
        cout << "error in opening file" << endl;
        return false;
    }
    int size = getfilesize(in);
    // uint32_t usz = htonl(size);
    // uint32_t tsize = ntohl(usz);
    // send filename to the client
    cout << "sendfilename to client " << filename << endl;
    /*if(sendto(false, filename) < 0) {
        cout << "sending error " << endl;
        return false;
    }*/
    int status = 0;
    int len = filename.size();
    // send filename size
    do {
        status = ::send(m_sockfd, (void*)&len, sizeof(int), 0);
    } while(status <= 0);
    // send filename
    do {
        status = ::send(m_sockfd, filename.c_str(), filename.size(), 0); //;sendto(false, filename);
    } while(status < 0);
    cout << "file size " << size << endl;

    string filesz = std::to_string(size);
    // send len
    len = filesz.size();
    do {
        status = ::send(m_sockfd, (void*)&len, sizeof(int), 0);
    } while(status <= 0);
    // send fille size
    do {
        status = sendto(false, filesz);
    } while(status <= 0);

    cout << savedir << endl;
    cout << "sendfile to client " << endl;

    long sentbytes = 0;
    long read;
    char buf[BUFSIZE];

    bzero(buf, BUFSIZE);
    while((read = in.read(buf, BUFSIZE).gcount()) > 0) {
        if(::send(m_sockfd, buf, read, 0) < 0) {
            return 1;
        }
        // cout<<" sent size " <<read<<endl;
        bzero(buf, BUFSIZE);
    }
    cout << "send completed" << endl;
    // close();
    return true;
}
string chatclient::setmyname(string name)
{
    m_name = name;
}

int chatclient::recvfile()
{
    int size = 0;
    string savedir = getpath();    // path of client.exe i.e ~/client
    string clientname = "client1"; // get clientname from map
    // create dir for client
    savedir = savedir + "_" + std::to_string(1); //~/client_1
    if(!createdir(savedir)) {
        cout << "Error in creating dir" << endl;
        return -1;
    }
    // wait for filename from server
    string filename;
    cout << " waiting to recieve filename " << endl;
    int len = 0;
    string res = "ok";
    int status = 0;
    do {
        status = ::recv(m_sockfd, &len, sizeof(int), 0);
    } while(status <= 0);
    // while(recvfrom(filename)<=0);
    // recv file name
    char buf[BUFSIZE];
    bzero(buf, BUFSIZE);
    do {
        status = ::recv(m_sockfd, buf, len, 0);
        // status =recvfrom(val);//::read(m_sockfd,val,sizeof(int));
    } while(status <= 0);

    filename = std::string(buf, len);
    cout << " Recieved file with name " << filename << endl;
    cout << "waiting for file size" << endl;
    string val;
    // uint32_t usz = 0;
    // size string len
    do {
        status = ::recv(m_sockfd, &len, sizeof(int), 0);
    } while(status <= 0);
    cout << "here" << endl;
    cout << len << endl;
    bzero(buf, BUFSIZE);
    do {
        status = ::recv(m_sockfd, buf, len, 0);
    } while(status <= 0);
    /*do{
        //status =::recv(m_sockfd,buf,len,0);
       status =recvfrom(val);//::read(m_sockfd,val,sizeof(int));
   }while(status <= 0);*/
    val = std::string(buf, len);
    size = std::stoi(val);

    // size = ntohl(usz);

    cout << val << endl;
    size = std::stoi(val);
    cout << " Recieved file with size " << size << endl;
    filename = savedir + "/" + filename;
    std::ofstream out(filename, std::ios::out | std::ios::binary);
    if(!out.is_open()) {
        cout << "fail to open " << endl;
        return -1;
    }
    int total = 0;
    cout << "Writing to file " << endl;
    int readsz = 0;

    while(total < size) {
        // string temp;
        readsz = ::recv(m_sockfd, buf, BUFSIZE, 0); //;recvfrom(temp);

        // cout<<"readsz "<<readsz<<endl;
        if(readsz < 0) {
            cout << "read error" << endl;
            return -1;
        }
        // cout<<"total" <<total<<endl;
        out.write(buf, readsz); //;out.write(temp.c_str(), temp.size());
        total += readsz;
        memset(buf, 0, readsz);
        if(readsz == 0)
            break;
    }

    cout << "read finished " << endl;
    out.close();
    return total;
}

int main(int argc, char** argv)
{

    chatclient clientsock;
    string hostname = "127.0.0.1";
    int port = 61823;
    clientsock.create();
    clientsock.connect(hostname, port);
    // string msg;
    // cin >> msg;
    // clientsock.sendto(false,msg);
    clientsock.recvfile();
    /*while(true){
    string resp;
    clientsock.recvfrom(resp);
    cout<<"From Server "<<resp<<endl;
    }*/
    clientsock.close();
    return 0;
}
