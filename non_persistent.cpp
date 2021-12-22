#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fstream>
#include <chrono>

using namespace std;
using namespace std::chrono;
const int backLog = 3;
const int maxDataSize = 1046576;

int main(){
    uint16_t serverPort=80;
    string serverIpAddr = "199.232.22.208";
    cout<<"Enter the ip address and port number of server"<<endl;
    cin>>serverIpAddr;
    cin>>serverPort;

    auto start = high_resolution_clock::now();

    //getting the objects from the "downloads.txt" file
    string row;
    ifstream f ("downloads.txt");
    if (f.is_open()){
        while(getline(f, row)){
           cout<<"Object requested : "<<row<<endl;

           //creating socket
           int clientSocketFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
           if(!clientSocketFd)
           {
              cout<<"Error creating socket"<<endl;
              exit(1);
           }
           struct timeval tv;
           tv.tv_sec = 2;
           tv.tv_usec = 0;
           setsockopt(clientSocketFd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

           struct sockaddr_in serverSockAddressInfo;
           serverSockAddressInfo.sin_family = AF_INET;
           serverSockAddressInfo.sin_port = htons(serverPort);
           inet_pton(AF_INET, serverIpAddr.c_str(), &(serverSockAddressInfo.sin_addr));

           memset(&(serverSockAddressInfo.sin_zero), '\0', 8);

           socklen_t sinSize = sizeof(struct sockaddr_in);
           int flags = 0;
           int dataRecvd = 0, dataSent = 0;
           struct sockaddr_in clientAddressInfo;
           char rcvDataBuf[maxDataSize], sendDataBuf[maxDataSize];
           string sendDataStr, rcvDataStr;

           //connecting to server
           int ret = connect(clientSocketFd, (struct sockaddr *)&serverSockAddressInfo, sizeof(struct sockaddr));
           if (ret<0)
           {
              cout<<"Error with server connection "<<endl;
              close(clientSocketFd);
              exit(1);
           }

           //extracting servername and objectname from url
           char req[maxDataSize];
           for(int i=0; i<row.length(); i++){
            req[i] = row[i];
           }

           int servernamestart;
           for(int i=0; i<strlen(req); i++){
              if(req[i] == '/' && req[i+1] == '/'){
                 servernamestart = i+2;
                 break;
              }
           }

           int objectstart;
           for(int j=servernamestart; j<strlen(req); j++){
              if(req[j]=='/'){
                 objectstart = j+1;
                 break;
              }
           }

           char servername[maxDataSize];
           int k=0;
           for(int j=servernamestart; j<objectstart-1; j++){
              servername[k] = req[j];
              k++;
           }

           char objectname[maxDataSize];
           k=0;
           for(int i=objectstart; i<strlen(req); i++){
              objectname[k] = req[i];
              k++;
           }

           //creating HTTP GET request
           char reqpart1[] = "GET /";

           char reqpart2[maxDataSize];
           for(int i=0; i<strlen(objectname); i++){
              reqpart2[i] = objectname[i];
           }

           char reqpart3[] = " HTTP/1.0\r\nHost: ";

           char reqpart4[maxDataSize];
           for(int i=0; i<strlen(servername); i++){
              reqpart4[i] = servername[i];
           }

           char reqpart5[] = "\r\n\r\n";

           char httpreq[strlen(reqpart1) + strlen(reqpart2) + strlen(reqpart3) + strlen(reqpart4) + strlen(reqpart5)];
           strcpy(httpreq, reqpart1);
           strcat(httpreq, reqpart2);
           strcat(httpreq, reqpart3);
           strcat(httpreq, reqpart4);
           strcat(httpreq, reqpart5);

           //sending the request to server
           dataSent = send(clientSocketFd, httpreq, strlen(httpreq), flags);
           if(dataSent<0){
              cout<<"Error sending message"<<endl;;
           }
           cout<<"HTTP request sent. Awaiting response...."<<endl;

           //receiving response from server
           char httpresponse[maxDataSize];
           while(1){
              memset(&rcvDataBuf, 0, maxDataSize);
              dataRecvd = recv(clientSocketFd, &rcvDataBuf, maxDataSize, flags);
              if(dataRecvd<=0){
                 break;
              }
              rcvDataStr = rcvDataBuf;
              strcpy(httpresponse+ strlen(httpresponse), rcvDataStr.c_str());
           }
           cout<<"HTTP response received"<<endl;

           //removinf header from response
           int i=0;
           int datastart = 0;
           while(i<strlen(httpresponse)){
              if(httpresponse[i] == '\r' && httpresponse[i+1] == '\n' && httpresponse[i+2] == '\r' && httpresponse[i+3] == '\n'){
                 datastart = i;
                 break;
              }
              i++;
           }

           char filename[maxDataSize];
           i=0;
           for(; i<strlen(objectname); i++){
            filename[i] = objectname[i];
           }
           filename[i] = '.';
           filename[i+1] = 'f';
           filename[i+2] = 'i';
           filename[i+3] = 'l';
           filename[i+4] = 'e';

           //saving the file
           ofstream outfile;
           outfile.open(filename);
           i = datastart+4;
           while(i<strlen(httpresponse)){
              outfile << httpresponse[i];
              i++;
           }
           cout<<"Object saved."<<endl;

           //closing the connecting since this is non persistent
           cout<<"All done closing socket now"<<endl;
           close(clientSocketFd);
        }
        f.close();
    }
    else{
        cout<<"Error while opening file"<<endl;
    }
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    cout<<"Time taken for execution: "<<duration.count()<<" microseconds"<< endl;
}
