#include <iostream>
#include <WS2tcpip.h>
#include <fstream>
#include <sstream>
#include <codecvt>
#include <vector>

#pragma comment (lib, "ws2_32.lib")

# define PORT 55000

using namespace std;

/* Check arguments */
bool checkArgs(int, char* []);
string getArgs(int, char* []);

/* Set up structure sockaddr */
void setSenderSock(sockaddr_in&, sockaddr_in&);
void setClientSock(sockaddr_in&);


/* Utility function */
string convertUint32ToStr(uint32_t);
uint32_t convertStrToUint32(string);
char* convertStringToChar(string);
vector<string> parseRecvData(string);
string parseUDPdataToString(char*, int, uint32_t, bool);
void exchangeInformation(SOCKET, sockaddr_in &, int, string&);
void receiveFile(SOCKET, sockaddr_in&, sockaddr_in&, int, string);
void writeToFile(string filename, string buf);
void finishTransferFile();

int main(int argc, char* argv[]) { 
    if (argc != 2) {
        cout << "Invalid the number of arguments !";
        return 0;
    }

    string output;
    if (checkArgs(argc, argv)) {
        output = getArgs(argc, argv);
    }
    else
        return 0;

    WSADATA data;
    WORD version = MAKEWORD(2, 2);
    // Start Winsock
    int wsOk = WSAStartup(version, &data);
    if (wsOk != 0) {
        cout << "Can't start Winsock! " << wsOk << "\n";
        return 0;
    }

    sockaddr_in client, sender;
    setClientSock(client);

    //Socket
    SOCKET in = socket(AF_INET, SOCK_DGRAM, 0);
     //Bind
    if (bind(in, (sockaddr*)&client, sizeof(client)) == SOCKET_ERROR) {
        cout << "Can't bind socket! " << WSAGetLastError() << "\n";
        return 0;
    }

    // Vector for store parsed data
    vector<string> arrArgs;
    // Store string after convert from char *
    string recvStr;

    cout << "===============\n"
        << "Starting receive file\n"
        << "===============\n";
    exchangeInformation(in, client, sizeof(client), recvStr);
    arrArgs = parseRecvData(recvStr);

    int buffer_size = atoi(arrArgs[0].c_str());
    uint32_t numberSegments = convertStrToUint32(arrArgs[1]);

    /* SET IP ADDRESS */
    // Assign IP address for sender
    setSenderSock(sender, client);
    receiveFile(in, client, sender, buffer_size, output);
    // Close socket
    closesocket(in);
    WSACleanup();
    return 0;
}

/*
*  Check the valid of parameters of the program
*  @ return
*       true if all parameter is valid
*       false if one of them is invalid
*/
bool checkArgs(int argc, char* argv[]) {
    string arg = argv[1];
    if (arg.find("--out=") != string::npos)
        if (arg.substr(0, 6) != "--out=") {
            cout << "Invalid syntax:" << arg << "\n";
            return false;
        }
        else
            cout << arg.substr(6, arg.length()) << "\n";
    else {
        cout << "Invalid syntax:" << arg << "\n";
        return false;
    }
    return true;
}

/*
*  Get path to file output
*  @ return
*       A string store path to output
*/
string getArgs(int argc, char* argv[])
{
    string arg = argv[1];
    return arg.substr(6, arg.length());
}
    
/*
*   Set IP and port for structure sender
*   @ param ipAddress
*       IP Address of Receiver
*/
void setSenderSock(sockaddr_in& sender, sockaddr_in& client) {
    // Create enough space to convert the address byte array
    char clientIp[256];
    ZeroMemory(clientIp, 256); // to string of characters
    // Convert from byte array to chars
    inet_ntop(AF_INET, &client.sin_addr, clientIp, 256);

    sender.sin_family = AF_INET; // AF_INET = IPv4 addresses
    sender.sin_port = htons(PORT); // Little to big endian conversion
    inet_pton(AF_INET, clientIp, &sender.sin_addr); // Convert from string to byte array
}

/*
*   Set IP and port for structure client
*/
void setClientSock(sockaddr_in& client) {
    client.sin_family = AF_INET; // AF_INET = IPv4 addresses
    client.sin_port = htons(PORT); // Little to big endian conversion
    inet_pton(AF_INET, "0.0.0.0", &client.sin_addr); // Convert from string to byte array
}


/*
*   Convert  unit32_t to string
*   @ return
*       A string number
*/
string convertUint32ToStr(uint32_t num) {
    std::stringstream ss;
    ss << num;
    std::string str;
    ss >> str;
    return str;
}

/*
*   Convert  string to unit32_t
*   @ return
*       A uint32_t number
*/
uint32_t convertStrToUint32(string numberStr)
{
    uint32_t number = static_cast<uint32_t>(std::stoul(numberStr));
    return number;
}

/*
*   Convert string to char *
*   @ return
*       A pointer point to array char
*/
char* convertStringToChar(string content) {
    char* writable = new char[content.size() + 1];
    copy(content.begin(), content.end(), writable);
    writable[content.size()] = '\0';
    return writable;
}

/*
*   Parse the UDP data to string
*   @ param buf
*       Char array
*   @ param buffer_size
*       the size of buffer
*   @ param idSegment
*       ID of the segment
*   @ param ack
*       A flag to identify the response of the other peer
*   @ return
*       A string's seperated by character ','
*/
string parseUDPdataToString(char* buf, int buffer_size, uint32_t idSegment, bool ack = 0) {
    string id = convertUint32ToStr(idSegment);
    string ackFlag;
    if (ack == 0)
        ackFlag = "0";
    else
        ackFlag = "1";
    string buffer(buf, buf + buffer_size);

    string result = id + "," + ackFlag + "," + buffer;
    return result;
}

/*
*   Parse the received data (string) to a list string (vector)
*   @ param recvData
*       Received data through socket
*   @ return
*       A vector stores data
*/
vector<string> parseRecvData(string recvData) {
    vector<string> arrArgs;
    // Character ',' is the delimiter
    string token, delimiter = ",";
    size_t pos = 0;

    // Preprocessing data
    recvData.erase(recvData.find_last_not_of(" \n\r\t") + 1);
    recvData.erase(std::remove(recvData.begin(), recvData.end(), '\0'), recvData.end());

    // Split the string
    while ((pos = recvData.find(delimiter)) != string::npos) {
        token = recvData.substr(0, pos);
        arrArgs.push_back(token);
        recvData.erase(0, pos + delimiter.length());
    }
    arrArgs.push_back(recvData);
    return arrArgs;
}

/*
*   Receive the basic information to Receiver such as buffer size, number of segments
*   @ param in
*       Socket
*   @ param client
*       structure sockaddr of Client
*   @ param clientLen
*       Length of structure client
*   @ param recvStr
*       String store data
*/
void exchangeInformation(SOCKET in, sockaddr_in &client, int clientLen, string &recvStr) {
    char tmpBuf[1025];
    // Receive buffer_size from Sender
    int bytesIn = recvfrom(in, tmpBuf, 1025, 0, (sockaddr*)&client, &clientLen);
    if (bytesIn == SOCKET_ERROR) {
        cout << "Error receiving from RecvFile " << WSAGetLastError() << "\n";
        exit(0);
    }
    recvStr.assign(tmpBuf, tmpBuf + 1025);
}
    
/*
*   Wait for receiving message from Sender, store data for writing in the next segment and 
    send ACK segment to Sender for identifying Receiver has received that segment with attached ID
*   @ param in
*       Socket
*   @ param client
*       structure sockaddr of Client
*   @ param sender
*       structure sockaddr of Sender
*   @ param clientLen
*       Length of structure client
*   @ param buffer_size
*       Buffer size
*   @ param pathFile
*       Path to file output
*/
void receiveFile(SOCKET in, sockaddr_in &client, sockaddr_in &sender, int buffer_size, string pathFile) {
    // Vector for store parsed data
    vector<string> arrArgs;
    // Store string after convert from char *
    string recvStr;

    int clientLength = sizeof(client);
    int senderLength = sizeof(sender);

    char* buf = new char[buffer_size];
    uint32_t segmentID, prevId = -2, id = 1;
    string prevData, str = "OK", udpData = "";

    while (true) {
        /* Receive message */
        cout << "===== RECEIVE SEGMENT " << prevId + 1 << " =====\n";
        ZeroMemory(&client, clientLength); // Clear the client structure
        ZeroMemory(buf, buffer_size); // Clear the receive buffer

        // Wait for message
        int bytesIn = recvfrom(in, buf, buffer_size, 0, (sockaddr*)&client, &clientLength);
        if (bytesIn == SOCKET_ERROR) {
            cout << "Error receiving from RecvFile " << WSAGetLastError() << "\n";
            continue;
        }

        // Parse buffer to string
        recvStr.assign(buf, buf + buffer_size);
        arrArgs = parseRecvData(recvStr);

        /* === Send ACK === */
        char* writable = convertStringToChar(str);
        segmentID = convertStrToUint32(arrArgs[0]);
        // Convert data to string
        udpData = parseUDPdataToString(writable, str.size() + 1, segmentID, 1);
        // Send response to Sender
        int sendOk = sendto(in, udpData.c_str(), udpData.size() + 1, 0, (sockaddr*)&sender, sizeof(sender));
        if (sendOk == SOCKET_ERROR)
        {
            cout << "Can't send ACK segment " << WSAGetLastError() << endl;
            continue;
        }

        cout << "===== SEND ACK " << segmentID << " =====\n";
        // don't forget to free the string after finished using it
        delete[] writable;

        if (prevId == segmentID - 1) {
            writeToFile(pathFile, prevData);
            if (arrArgs[2] == "fjnjsh_tr4nf3r_fil3!") {
                finishTransferFile();
                cout << "Received " << segmentID << " segments\n";
                break;
            }
        }

        // Store current data
        prevId = segmentID;
        prevData = arrArgs[2];
        // Clear the vector for next chunk
        arrArgs.clear();
    }
    // Free the memory
    delete[] buf;
}

/*
*   Write file utf-8
*   @ param filepath
*       Path to file (output)
*   @ param buf
*       Buffer for writing to file
*/
void writeToFile(string filepath, string buf)
{
    wofstream wof;
    wof.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t>));
    wof.open(filepath, std::wios::app);
    wof << buf.c_str();
    wof.close();
}

/*
*   Print the message
*/
void finishTransferFile() {
    cout << "\n---------------------\n"
        << "Transfer file successfully\n"
        << "---------------------\n";
}