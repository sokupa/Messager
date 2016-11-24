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

#define BUFSIZE 1024
using std::endl;
using std::cout;
using std::cin;
using std::string;
using std::unordered_map;
using std::thread;
using std::vector;
using std::stringstream;

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
    int sendto(bool isfile ,char (&buf)[BUFSIZE], string msg);
    int recvfrom(int&);
    int recvfrom(string&);
    int recvfrom(char (&buf)[BUFSIZE],int len);
    void close();
    bool sendfile();
    int recvfile();
    int getfilesize(std::ifstream&);
    string getpath();
    void setname();
    bool createdir(string);
    bool validatecmd(int&, string);
    vector<string> parse(string);
    string getname();
    static int handlerecv(chatclient);
    static void handlesend(chatclient);
};

unordered_map<string, int> chatclient::cmdmap = { { "BC", 0 }, { "BC-FILE", 1 }, { "UC", 2 }, { "UC-FILE", 3 },
    { "BLC", 4 }, { "BLC-FILE", 5 } };

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
    cout<<"send to before "<<out<<endl; 
    do {
        status = ::send(m_sockfd, out.c_str(), out.size(), MSG_NOSIGNAL);
    } while(status <= 0);
    cout<<"after send"<<endl;
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

int chatclient::sendto(bool isfile ,char (&buf)[BUFSIZE], string msg)
{
    int len = msg.size();
    //cout<<"buf send before "<<buf<<endl;
    sendto(false,len);//send buf len
    int status = 0;
    strcpy(buf,msg.c_str()); 
    do{
        status = ::send(m_sockfd,buf,len,0);
    }while(status<=0);
    //cout<<"buf send after "<<endl;
    return status;
}
int chatclient::recvfrom(string& inp)
{
    char cstr[100];
int size = 0;
    memset(cstr, 0, 100);
//cout<<"recv from before "<<endl;
    do {
        size = ::recv(m_sockfd, cstr, 100, 0);
    } while(size <= 0);
    inp = std::string(cstr, size);
    //cout<<"recv from after "<<size<< endl;
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
        cout<<"socket close"<<endl;
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
    cout << "sendfilename to client " << filename << endl;
    int status = 0;
    int len = filename.size();
    // send filename size
    status = sendto(false, len);
    // send filename
    status = sendto(false, filename); //;sendto(false, filename);
    cout << "file size " << size << endl;
    string filesz = std::to_string(size);
    // send len
    len = filesz.size();
    status = sendto(false, len);
    // send fille size
    status = sendto(false, filesz);
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
    return true;
}

int chatclient::recvfile()
{
    int size = 0;
    string savedir = getpath(); // path of client.exe i.e ~/client
    string clientname = m_name; // get clientname from map
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
    status = recvfrom(len);
    char buf[BUFSIZE];
    bzero(buf, BUFSIZE);
    do {
        status = ::recv(m_sockfd, buf, len, 0);
    } while(status <= 0);

    filename = std::string(buf, len);
    cout << " Recieved file with name " << filename << endl;
    cout << "waiting for file size" << endl;
    string val;
    // uint32_t usz = 0;
    // size string len
    status = recvfrom(len);
    cout << "here" << endl;
    cout << len << endl;
    bzero(buf, BUFSIZE);
    do {
        status = ::recv(m_sockfd, buf, len, 0);
    } while(status <= 0);
    val = std::string(buf, len);
    size = std::stoi(val);
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
bool chatclient::validatecmd(int& cmdid, string command)
{
    auto it = cmdmap.find(command);
    if(it != cmdmap.end()) {
        cmdid = it->second;
        return true;
    }
    return false;
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
string chatclient::getname()
{
    return m_name;
}

void chatclient::handlesend(chatclient clientsock)
{
    string command;
    while(1) {
        cout << "enter command" << endl;
        getline(std::cin,command);
        //cout << "entered command" <<command<<endl;
        // string command = argv[1];
        vector<string> args;
        args = clientsock.parse(command);
        int cmdlen = args.size();
        if(cmdlen <= 0) {
            cout << "Entered empty command" << endl;
            continue;
        }
        int cmdid = 0;
        command = args[0];
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
        //cout<<" command "<< args[0] << " command id " <<cmdid<<endl;
        int status = 0;
        char buf[BUFSIZE];
        //send len
        //clientsock.sendto(false,command.size());
        status = clientsock.sendto(false, buf,command);
        switch(cmdid) {
        // message
        // Broadcast
        case 0: {
            string msg = args[1];
            cout<<" Message " <<args[1]<<endl;
            while(msg.empty()) {
                cout << " enter message to broadcast" << endl;
                cin >> msg;
            }
            // send the message
            //clientsock.sendto(false,msg.size());
            status = clientsock.sendto(false, buf,msg);
            //cout<<"sent "<<endl;
        } break;
        // unicast
        case 2:
        case 4: {
            string msg = args[1];
            string to = args[2];
            while(msg.empty()) {
                cout << " enter message to broadcast" << endl;
                cin >> msg;
            }
            while(to == clientsock.getname() || to.size() == 0) {
                cout << " trying to send message to self or none" << endl;
                cout << " enter client as client_<id>" << endl;
                cin >> to;
            }
            msg = args[1] + "###" + args[2]; // message###client
            // send the message
            status = clientsock.sendto(false, msg);
        } break;
        // Blockcast
        /* case 4: {
             string msg = argv[2];
             string toblock = argv[3];
             while(msg.empty()) {
                 cout << " enter message to broadcast" << endl;
                 cin >> msg;
             }
             // send the message
             do {
                 status = clientsock.sendto(false, msg);
             } while(status <= 0);
         } break;*/
        case 1:
            break;
        case 3:
            break;

        case 5:
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
        //clientsock.recvfrom(cmd);
        cout<<cmd<<endl;
        int cmdid;
        if(clientsock.validatecmd(cmdid, cmd) == false) {
            cout << " Invalid command from server";
            // notify server to retry
            //string res = "Invalid command";
            //status = clientsock.sendto(false, res);
            //continue; // restart from begin
            cin >> cmd;
       }
        switch(cmdid) {
        // recieve msg
        case 0:
        case 2:
        case 4: {
            string msg;
            clientsock.recvfrom(len);
            clientsock.recvfrom(buf, len);
            msg = string(buf,len);
            cout << msg << endl;
        } break;
        // recieve file
        case 1:
        case 3:
        case 5:
            clientsock.recvfile();
            break;
        }
    }
}
void chatclient::setname()
{
    // ask server to get name
    int id = 0;
    recvfrom(id);
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
    thread receiver(chatclient::handlerecv,(clientsock));
    send.join();
    receiver.join();
    //clientsock.close();
    return 0;
}
