#include <algorithm>
#include <arpa/inet.h>
#include <arpa/inet.h>
#include <cstring> //for memset
#include <ctime>
#include <errno.h>
#include <fstream>
#include <iostream>
#include <list>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h> //for mkdir
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>
/* Change class hierarchy later, common socket code of client and
 * server can be moved in generic socket
 * */
#define MAXCLIENT 100
#define DEBUG_LOG false
#define BASE 6000
#define MAXGROUP 5
#define BUFSIZE 256
using std::endl;
using std::cout;
using std::cin;
using std::string;
using std::thread;
using std::unordered_set;
using std::unordered_map;
using std::pair;
using std::vector;
using std::make_pair;
using std::to_string;
/*class genericsocket(){

};*/
class chatserver
{
private:
    int m_sockfd; // socket fd
    static int sockid;
    static int clientid; // no need to be atomic as accpet will happen in a single thread
    sockaddr_in m_addr;  // server address
    static unordered_set<int> sockets;
    static vector<vector<pair<int, int> > > grouplist;
    static unordered_map<int, pair<int, int> > clientmap;
    static unordered_map<string, int> cmdmap;
    static unordered_map<string, int> cnametosock;

public:
    chatserver()
        : m_sockfd(0)
    {
        memset(&m_addr, 0, sizeof(m_addr));
    }
    ~chatserver()
    {
        // close();
    }
    bool create();
    bool bind(int port);
    bool listen();
    bool accept(chatserver&);
    int sendto(bool isfile, string out);
    int sendto(bool, int);
    int sendto(int, bool, string out); // send to other sock
    int sendto(int, bool, int);
    int sendto(bool isfile, char (&buf)[BUFSIZE], string msg);
    int sendto(int sock, bool isfile, char (&buf)[BUFSIZE], string msg);
    int recvfrom(int&);
    int recvfrom(string&);
    int recvfrom(char (&buf)[BUFSIZE], int len); // to do
    void close();
    void sendtoAll();              // broadcast
    void sendtoAllfile();          // file send to all
    void sendtogroup(chatserver&); // sendto group of client
    void addtoset(const chatserver&);
    void blockcast();
    void blockcastfile();
    bool sendfile(string, int);
    int recvfile(string&);
    string getpath();
    void unicast();
    void unicastfile();
    int getclientid();
    void sendclientid(chatserver&);
    int getfilesize(std::ifstream& file);
    bool createdir(string);
    bool validatecmd(int&, string&);
    void printlog(string);
};
unordered_set<int> chatserver::sockets;
vector<vector<pair<int, int> > > chatserver::grouplist(MAXGROUP);
unordered_map<int, pair<int, int> > chatserver::clientmap; // store clientid, map pair
int chatserver::clientid = 0;
unordered_map<string, int> chatserver::cnametosock; // map clientname to sock
unordered_map<string, int> chatserver::cmdmap = { { "BC", 0 }, { "BC-FILE", 1 }, { "UC", 2 }, { "UC-FILE", 3 },
    { "BLC", 4 }, { "BLC-FILE", 5 } };

void chatserver::printlog(string str)
{
    if(DEBUG_LOG)
        cout << str << endl;
}
bool chatserver::create()
{
    m_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(m_sockfd == -1)
        return false;
}

bool chatserver::bind(int port)
{
    if(m_sockfd == -1)
        return false;
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = htons(port);
    m_addr.sin_addr.s_addr = INADDR_ANY; // inet_addr("127.0.0.1")

    return ::bind(m_sockfd, (struct sockaddr*)&m_addr, sizeof(m_addr)) == -1 ? false : true;
}
bool chatserver::listen()
{
    if(m_sockfd == -1)
        return false;
    return (::listen(m_sockfd, MAXCLIENT) == -1) ? false : true;
}
bool chatserver::accept(chatserver& newsock)
{
    int size = sizeof(m_addr);
    newsock.m_sockfd = ::accept(m_sockfd, (sockaddr*)&m_addr, (socklen_t*)&size);
    if(newsock.m_sockfd <= 0)
        return false;
    printlog("serverfd "+std::to_string(m_sockfd));
    printlog("acceptfd "+std::to_string(newsock.m_sockfd));
    std::srand(std::time(0));
    int groupid = std::rand() % MAXGROUP;
    clientmap[newsock.m_sockfd] = make_pair(groupid, clientid);
    cnametosock["client_" + std::to_string(clientid)] = newsock.m_sockfd;
    grouplist[groupid].push_back(make_pair(clientid++, newsock.m_sockfd));
    addtoset(newsock);
    return true;
}

int chatserver::sendto(bool isfile, string const out)
{
    int status = 0;
    do {
        status = ::send(m_sockfd, out.c_str(), out.size(), MSG_NOSIGNAL);
    } while(status <= 0);
    return status;
}

int chatserver::sendto(bool isfile, int const out)
{
    int status = 0;
    int val = out;
    printlog("before sendto int "+to_string(out));
    do {
        status = ::send(m_sockfd, &val, sizeof(int), 0);
    } while(status <= 0);
    printlog("after sendto int");
    return status;
}

int chatserver::sendto(int sock, bool isfile, string const out)
{
    int status = 0;
    do {
        status = ::send(sock, out.c_str(), out.size(), MSG_NOSIGNAL);
    } while(status <= 0);
    return status;
}

int chatserver::sendto(int sock, bool isfile, char (&buf)[BUFSIZE], string const msg)
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
int chatserver::sendto(int sock, bool isfile, int const out)
{
    int status = 0;
    do {
        status = ::send(sock, &out, sizeof(int), MSG_NOSIGNAL);
    } while(status <= 0);
    return status;
}

int chatserver::sendto(bool isfile, char (&buf)[BUFSIZE], string msg)
{
    int len = msg.size();
    int status = 0;
    sendto(false, len); // send buf len
    strcpy(buf, msg.c_str());
    do {
        status = ::send(m_sockfd, buf, len, 0);
    } while(status <= 0);
    return status;
}
int chatserver::recvfrom(string& inp)
{
    if(m_sockfd < 0) {
        cout << " Not valid sockid " << endl;
        return -1;
    }
    char cstr[100];
    memset(cstr, 0, 100);
    int size = 0;
    do {
        size = ::recv(m_sockfd, cstr, 100, 0);
    } while(size <= 0);
    inp = std::string(cstr, size);
    return size;
}

int chatserver::recvfrom(char (&buf)[BUFSIZE], int len)
{
    int status = 0;
    bzero(buf, BUFSIZE);
    do {
        status = ::recv(m_sockfd, buf, len, 0);
    } while(status <= 0);
    return status;
}

int chatserver::recvfrom(int& inp)
{
    int status = 0;
    int val = 0;
    do {
        status = ::recv(m_sockfd, &val, sizeof(int), 0);
    } while(status <= 0);
    inp = val;
    return status;
}
void chatserver::close()
{
    if(m_sockfd == -1)
        return;
    cout << "closing " << endl;
    ::close(m_sockfd);
}

void chatserver::addtoset(const chatserver& clientsock)
{
    sockets.insert(clientsock.m_sockfd);
}

void chatserver::sendtoAll()
{
    if(m_sockfd == -1) {
        return;
    }
    string msg;
    int len = 0;
    char buf[BUFSIZE];
    recvfrom(len);
    recvfrom(buf, len);
    msg = string(buf, len);
    for(auto sock : sockets) {
        if(sock != m_sockfd) // don't broadcast to self
        {
            sendto(sock, false, buf, "BC");
            sendto(sock, false, buf, msg);
        }
    }
}

void chatserver::sendtoAllfile()
{
    if(m_sockfd == -1) {
        return;
    }
    string filename;
    if(recvfile(filename) <= 0) {
        cout << "error in reciving file " << endl;
        return;
    }
    char buf[BUFSIZE];
    printlog("file recieved " +filename);
    for(auto sock : sockets) {
        if(sock != m_sockfd) // don't broadcast to self
        {
            sendto(sock, false, buf, "BC-FILE");
            sendfile(filename, sock);
        }
    }
}
void chatserver::unicast()
{
    string msg;
    char buf[BUFSIZE];
    int len = 0;
    recvfrom(len);
    recvfrom(buf, len);
    msg = string(buf, len);
    printlog("unicast message "+msg);
    string to_client = msg.substr(msg.find_last_of("###") + 1);
    msg = msg.substr(0, msg.find_first_of("###"));
    printlog(" to_client "+to_client);
    auto it = cnametosock.find(to_client);
    if(it == cnametosock.end()) {
        cout << "Invalid client id" << endl;
        // client sent invalid name or other client not yet connected
        // no self id check as it is handled by client itself
        sendto(false, buf, "$NOK$");              // notify client not valid id
        sendto(false, buf, "Not a valid client"); // send error
        return;
    }
    sendto(it->second, false, buf, "UC");
    sendto(it->second, false, buf, msg);
}

void chatserver::unicastfile()
{
    if(m_sockfd == -1) {
        return;
    }
    string to_client;
    int len = 0;
    char buf[BUFSIZE];
    string filename;
    recvfrom(len);
    recvfrom(buf, len);
    printlog("unicast File ");
    printlog("len " +to_string(len)+" buf "+buf);
    to_client = string(buf, len);
    auto it = cnametosock.find(to_client);
    int tosend = m_sockfd;
    if(it == cnametosock.end()) {
        cout << "Invalid client id" << endl;
        // client sent invalid name or other client not yet connected
        // no self id check as it is handled by client itself
        sendto(false, buf, "$NOK$");              // notify client not valid id
        sendto(false, buf, "Not a valid client"); // send error
        return;
    }
    sendto(it->second, false, buf, "UC-FILE");
    if(recvfile(filename) > 0) {
        sendfile(filename, it->second);
    }
}
void chatserver::blockcast()
{
    if(m_sockfd == -1) {
        return;
    }
    string msg;
    int len = 0;
    char buf[BUFSIZE];
    recvfrom(len);
    recvfrom(buf, len);
    msg = string(buf, len);
    string to_client = msg.substr(msg.find_last_of("###") + 1);
    msg = msg.substr(0, msg.find_first_of("###"));
    auto it = cnametosock.find(to_client);
    int block = m_sockfd;
    printlog("blockcast "+msg+" to block "+to_client );
    if(it != cnametosock.end()) {
        block = it->second;
    }
    for(auto sock : sockets) {
        if(sock == m_sockfd || sock == block) // don't broadcast to self
            continue;
        sendto(sock, false, buf, "BLC");
        sendto(sock, false, buf, msg);
    }
}

void chatserver::blockcastfile()
{
    if(m_sockfd == -1) {
        return;
    }
    string to_client;
    int len = 0;
    char buf[BUFSIZE];
    recvfrom(len);
    recvfrom(buf, len);
    printlog("len "+to_string(len)+" buf "+buf);
    to_client = string(buf, len);
    auto it = cnametosock.find(to_client);
    int block = m_sockfd;
    string filename;
    if(recvfile(filename) <= 0) {
        cout << "Error in recieving " << endl;
        return;
    }
    if(it != cnametosock.end()) {
        block = it->second;
    }
    for(auto sock : sockets) {
        if(sock == m_sockfd || sock == block) // don't broadcast to self
            continue;
        sendto(sock, false, buf, "BLC-FILE");
        sendfile(filename, sock);
    }
}
void chatserver::sendtogroup(chatserver& server)
{
    if(m_sockfd == -1) {
        return;
    }
    string msg;
    recvfrom(msg); // recieve message from client that is broadcasting
    int mygroupid = clientmap[m_sockfd].first;
    vector<pair<int, int> > groupmem = grouplist[mygroupid];
    for(auto it : groupmem) {
        if(it.second != m_sockfd)
            ::send(it.second, msg.c_str(), msg.size(), MSG_NOSIGNAL);
    }
}

string chatserver::getpath()
{
    char buf[100];
    int len = readlink("/proc/self/exe", buf, 100);
    return string(buf, (len > 0 ? len : 0));
}

bool chatserver::createdir(string path)
{
    bool success = false;
    printlog("creating  dir "+path);
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
int chatserver::getfilesize(std::ifstream& file)
{
    file.ignore(std::numeric_limits<std::streamsize>::max());
    int size = file.gcount();
    file.clear(); //  Since ignore will have set eof.
    file.seekg(0, std::ios_base::beg);
    return size;
}

bool chatserver::sendfile(string filename, int sockfd)
{
    string savedir = getpath(); // return path of current exe running
    savedir = savedir + "_dir";
    createdir(savedir);
    savedir = savedir + "/" + filename;
    printlog("Saved here: " + savedir );
    std::ifstream in(savedir, std::ios::in | std::ios::binary);
    if(!in) {
        cout << "error in opening file" << endl;
        return false;
    }
    int size = getfilesize(in);
    printlog("sendfilename to client "+filename);
    int status = 0;
    int len = 0;
    char buf[BUFSIZE];
    bzero(buf, BUFSIZE);
    // send filename
    sendto(sockfd, false, buf, filename);
    // cout << "file size " << size << endl;
    string filesz = std::to_string(size);
    // send filesize
    sendto(sockfd, false, buf, filesz);
    printlog("sending file ");
    long sentbytes = 0;
    long read;
    while((read = in.read(buf, BUFSIZE).gcount()) > 0) {
        if(::send(sockfd, buf, read, 0) < 0) {
            return 1;
        }
        bzero(buf, BUFSIZE);
    }
    printlog("send completed");
    return true;
}

void chatserver::sendclientid(chatserver& server)
{
    if(m_sockfd == -1) {
        cout << "Invalid client" << endl;
        return;
    }
    auto it = clientmap.find(m_sockfd);
    int id = it->second.second; // stores pair of group and id
    // send id to client;
    sendto(false, id);
}

int chatserver::recvfile(string& tosend)
{
    int size = 0;
    string savedir = getpath(); // path of client.exe i.e ~/client
    savedir = savedir + "_dir";
    if(!createdir(savedir)) {
        cout << "Error in creating dir" << endl;
        return -1;
    }
    // wait for filename from server
    string filename;
    char buf[BUFSIZE];
    bzero(buf, BUFSIZE);
    printlog(" waiting to recieve filename ");
    int len = 0;
    int status = 0;
    recvfrom(len);
    recvfrom(buf, len);
    filename = std::string(buf, len);
    tosend = filename;
    printlog(" Recieved file with name " + filename );
    if(filename == "Invalid file"){
        cout<<"Not a valid filename "<<endl;
        return -1;
    }
    printlog("waiting for file size" );
    string val;
    recvfrom(len);
    printlog(to_string(len));
    bzero(buf, BUFSIZE);
    recvfrom(buf, len);
    val = std::string(buf, len);
    size = std::stoi(val);
    size = std::stoi(val);
    printlog(" Recieved file with size "+to_string(size));
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
        readsz = ::recv(m_sockfd, buf, BUFSIZE, 0);
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

bool chatserver::validatecmd(int& cmdid, string& command)
{
    auto it = cmdmap.find(command);
    if(it != cmdmap.end()) {
        cmdid = it->second;
        return true;
    }
    return false;
}
void handler(chatserver newsock)
{
    int len = 0;
    string command;
    char buf[BUFSIZE];
    try{
    while(true) {
        // wait for command from client
        newsock.printlog("waiting for command ");
        newsock.recvfrom(len);
        bzero(buf, BUFSIZE);
        newsock.recvfrom(buf, len);
        command = string(buf, len);
        newsock.printlog("from client " + command);
        int cmdid = 0;
        if(newsock.validatecmd(cmdid, command) == false)
            continue;
        switch(cmdid) {
        case 0: {
            newsock.sendtoAll();
        } break;
        case 1: {
            newsock.sendtoAllfile();
        } break;
        case 2: {
            newsock.unicast();
        } break;
        case 3: {
            newsock.unicastfile();
        } break;
        case 4: {
            newsock.blockcast();
        } break;
        case 5: {
            newsock.blockcastfile();
        }
        break;
        default :
           break;
        }
    }
    } catch(std::exception e){
        cout<< e.what();
        handler(newsock); //restart
    }
    cout << "exit handler " << endl;
    newsock.close(); // exit loop only on error
}
int main(int argc, char** argv)
{
    int port = 61823; // argv [1];
    chatserver server;
    server.create();
    server.bind(port);
    server.listen();
    while(1) {
        chatserver newsock;
        if(!server.accept(newsock)) {
            cout << "conn err " << errno << endl;
            continue;
        }
        cout << "Accepted new client " << endl;
        newsock.sendclientid(server);
        thread th(handler, (newsock));
        th.detach();
    }
}
