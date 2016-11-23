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
    int sendto(bool isfile, string const out); // todo
    int recvfrom(string& in);                  // to do
    void close();
    void sendtoAll(chatserver&);   // broadcast
    void sendtogroup(chatserver&); // sendto group of client
    void addtoset(const chatserver&);
    bool sendfile();
    int recvfile();
    string getpath();
    string clientname();
    int getfilesize(std::ifstream& file);
    bool createdir(string);
};
unordered_set<int> chatserver::sockets;
vector<vector<pair<int, int> > > chatserver::grouplist(MAXGROUP);
unordered_map<int, pair<int, int> > chatserver::clientmap; // store clientid, map pair
// int chatserver::sockid = BASE+MAXCLIENT;
int chatserver::clientid = 0;

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
    // cout<<"serverfd "<<m_sockfd<<endl;
    // cout << "acceptfd " << newsock.m_sockfd <<endl;
    std::srand(std::time(0));
    int groupid = std::rand() % MAXGROUP;
    clientmap[newsock.m_sockfd] = make_pair(groupid, clientid);
    // test group rand generate no from 0 to 4 and alot
    grouplist[groupid].push_back(make_pair(clientid++, newsock.m_sockfd));
    return true;
}

int chatserver::sendto(bool isfile, string const out)
{
    return ::send(m_sockfd, out.c_str(), out.size(), MSG_NOSIGNAL);
}
void chatserver::close()
{
    if(m_sockfd == -1)
        return;
    ::close(m_sockfd);
}
int chatserver::recvfrom(string& inp)
{
    char cstr[100];
    memset(cstr, 0, 100);
    int size = recv(m_sockfd, cstr, 100, 0);
    if(size == -1)
        return 0;
    inp = cstr;
    return size;
}

void chatserver::addtoset(const chatserver& clientsock)
{
    sockets.insert(clientsock.m_sockfd);
}

void chatserver::sendtoAll(chatserver& server)
{
    if(m_sockfd == -1) {
        return;
    }
    string msg;
    recvfrom(msg); // recieve message from client that is broadcasting
    cout << "sendtoALl bmsg " << msg << endl;
    cout << "sendtoALl socketssize " << sockets.size() << endl;
    for(auto sock : sockets) {
        cout << "sockfd sendto ALL " << sock << endl;
        if(sock != m_sockfd) // don't broadcast to self
            ::send(sock, msg.c_str(), msg.size(), MSG_NOSIGNAL);
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
void handlereq(chatserver newsock)
{
    string data = "hello from server";
    string c_data;
    newsock.recvfrom(c_data);
    cout << "From client " << c_data << endl;
    newsock.sendto(false, data);
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
int chatserver::getfilesize(std::ifstream& file)
{
    file.ignore(std::numeric_limits<std::streamsize>::max());
    int size = file.gcount();
    file.clear(); //  Since ignore will have set eof.
    file.seekg(0, std::ios_base::beg);
    return size;
}

bool chatserver::sendfile()
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
string chatserver::clientname()
{
    string name;
    if(m_sockfd == -1) {
        cout << "Invalid client" << endl;
        return name;
    }
    int clientid = 1;
    return "client" + std::to_string(clientid);
}

int chatserver::recvfile()
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
    int port = 61823; // argv [1];
    chatserver server;
    server.create();
    server.bind(port);
    server.listen();
    // server.recvfile();
    // newsock.sendfile();
    while(1) {
        chatserver newsock;
        if(!server.accept(newsock)) {
            cout << "conn err " << errno << endl;
            // return false;
        }
        string clientname = newsock.clientname();
        // server.addtoset(newsock);
        // newsock.sendtoAll(server);
        newsock.sendfile();
        // thread th(handlereq,newsock);
        // th.detach();
    }
}
