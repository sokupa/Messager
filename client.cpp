#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <thread>
#include <list>
#include <algorithm>
#include <unordered_set>
#include <sys/stat.h> //for mkdir
#include <sys/types.h>


using std::endl;
using std::cout;
using std::cin;
using std::string;

class chatclient{
    private:
    int m_sockfd; // server port
    struct sockaddr_in m_addr; // server address
    string m_name;
public:
     chatclient():m_sockfd(0){
       memset (&m_addr, 0,sizeof ( m_addr ) );   
    }
     bool create();       
     bool bind(int port);
     bool connect(const string & , const int);
     bool sendto(bool isfile, string out); // todo 
     int recvfrom(string &) ;// to do
     void close();    
    bool sendfile();
    int recvfile();
    string getpath(); 
    string setmyname(string);
    bool createdir(string);
};

bool chatclient::create(){
    m_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(m_sockfd == -1)
        return false;
}

bool chatclient::bind(int port){
if(m_sockfd == -1)
    return false;
  m_addr.sin_family = AF_INET;
  m_addr.sin_addr.s_addr = INADDR_ANY;
  m_addr.sin_port = htons ( port );
  return ::bind(m_sockfd,( struct sockaddr * ) &m_addr,sizeof ( m_addr )) == -1?false:true;
}

bool chatclient::connect(const string &servadd , const int servport){
    if(m_sockfd == -1) 
        return false;
     m_addr.sin_family = AF_INET;
     m_addr.sin_port = htons ( servport );
     inet_pton(AF_INET ,servadd.c_str(),&m_addr.sin_addr);     
     int retval = ::connect(m_sockfd,( sockaddr * ) &m_addr,sizeof(m_addr));
     return retval==0?true:false;
}

bool chatclient::sendto(bool isfile, string out){
    return send(m_sockfd,out.c_str(),out.size(), MSG_NOSIGNAL) == -1?false:true;
}

int chatclient::recvfrom(string& inp){
    char cstr[100];
    memset(cstr, 0,100);
    int size = recv(m_sockfd,cstr,100,0);
    if(size == -1) return 0;
    inp = cstr;
    return size;
}

void chatclient::close(){
    if(m_sockfd == -1)
        return ;
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
    cout<<"createdir "<<path<<endl;
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
    cout<<success<<endl;
    return success;
}

bool chatclient::sendfile()
{
    string savedir = getpath(); // return path of current exe running
     cout<<savedir<<endl;
    cout << "enter file name" << endl;
    string filename;
    cin >> filename;
    createdir(savedir+"_dir");
    // send filename to the client
    int sendfd = m_sockfd; // sockid to be added as per group or indi
    if(!sendto(false, filename)) {
        cout << "sending error " << endl;
        return false;
    }
    // open the file in binary mode
    savedir = savedir + "/" + filename;
    cout<<savedir<<endl;
    std::ifstream in(savedir, std::ios::in | std::ios::binary);
    if(!in) {
        cout << "error in opening file" << endl;
        return false;
    }

    while(true) {
        char buf[1024];
        int read = in.read(buf, 1024).gcount();
        if(in.eof()) {
            break;
        }
        int sentbytes = 0;
        string msg(buf);
        if(!sendto(false, msg)) {
            cout << "error in sending file" << endl;
            return false;
        }
    }
    return true;
}
string chatclient::setmyname(string name)
{
   m_name = name;
}

int chatclient::recvfile()
{
    string savedir = getpath();//path of client.exe i.e ~/client
    string clientname = "client1"; // get clientname from map
    // create dir for client
    savedir = savedir + "_" + std::to_string(1); //~/client_1
    if(!createdir(savedir)) {
        cout << "Error in creating dir" << endl;
        return -1;
    }
    // wait for filename from server
    string filename;
    cout<<" waiting to recieve filename "<<endl;
    while(recvfrom(filename)<=0);
    /*if(recsz <= 0) {
        // error or empty filename
        cout << " Filename Read error" << endl;
        return -1;
    }*/
    cout<<" Recieved file with name "<<filename<<endl;
    cout<<"saved dir client "<<savedir<<endl;
    filename = savedir+"/"+filename;
    std::ofstream out(filename, std::ios::out | std::ios::binary);
    if(!out.is_open()) {
        cout<<"fail to open "<<endl;
        return -1;
    }
    long total;
    cout<<"Writing to file "<<endl;
    int readsz = 0;
    do {
        string temp;
        readsz = recvfrom(temp);
        total += readsz;
        cout<<"readsz "<<readsz<<endl;
        if(readsz == 0)
            break;
        if(readsz < 0) {
            cout << "read error" << endl;
            return -1;
        }
        //cout<<temp<<endl;
        out.write(temp.c_str(), temp.size());
    } while(readsz >0);
    cout<<"read finished "<<endl;
    out.close();
    return total;
}

int main(int argc, char** argv){

    chatclient clientsock;
    string hostname = "127.0.0.1";
    int port = 61823;
    clientsock.create();
    clientsock.connect(hostname, port);
    //string msg;
    //cin >> msg;       
    //clientsock.sendto(false,msg);
    clientsock.recvfile();
    /*while(true){ 
    string resp;
    clientsock.recvfrom(resp);
    cout<<"From Server "<<resp<<endl;
    }*/
    clientsock.close();
    return 0;
}
