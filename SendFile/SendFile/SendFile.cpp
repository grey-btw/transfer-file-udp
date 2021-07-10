#include <iostream>
#include <string>
#include <vector>
#include <WS2tcpip.h>
#include <filesystem>
#include <sstream>
#include <fstream>
#include <codecvt>

#pragma comment (lib, "ws2_32.lib")

# define PORT 55000

using namespace std;

// Put arguments into list argument
bool checkArgs(int, char* []);
vector<string> getArgs(int, char* []);

// Set up structure sockaddr
void setReceiverSock(sockaddr_in&, const char*);
void setClientSock(sockaddr_in&);

/* Utility function */
string convertUint32ToStr(uint32_t);
uint32_t convertStrToUint32(string);
char* convertStringToChar(string);
int getFileSize(string pathFile);
string parseUDPdataToString(char*, int, uint32_t, bool);
vector<string> parseRecvData(string);

/* Main function */
void readFileChunk(SOCKET, sockaddr_in, sockaddr_in&, string, int);
void exchangeInformation(SOCKET out, sockaddr_in server, string filename, int buffer_size, int &numSegments);
void lastSegment(SOCKET, sockaddr_in, sockaddr_in&, int, int);
void finishTransferFile();

int main(int argc, char* argv[]) {
    if (argc != 4) {
        cout << "Invalid the number of arguments!";
        return 0;
    }

    vector<string> listArg;

    if (checkArgs(argc, argv)) {
        listArg = getArgs(argc, argv);
    }
    else
        return 0;

    WSADATA data;
    WORD version = MAKEWORD(2, 2);
    // Start WinSock
    int wsOk = WSAStartup(version, &data);
    if (wsOk != 0)
    {
        // Not ok! Get out quickly
        cout << "Can't start Winsock! " << wsOk;
        return 0;
    }

    // Create a hint structure for the receiver
    sockaddr_in receiver;
    setReceiverSock(receiver, listArg[0].c_str());
     //Create a hint structure for the other peer
    sockaddr_in client;
    setClientSock(client);
    // Socket creation, note that the socket type is datagram
    SOCKET out = socket(AF_INET, SOCK_DGRAM, 0);

    // Bind
    if (bind(out, (sockaddr*)&client, sizeof(client)) == SOCKET_ERROR) {
        cout << "Can't bind socket!!! " << WSAGetLastError() << "\n";
        return 0;
    }

    readFileChunk(out, receiver, client, listArg[1], stoi(listArg[2]));
    finishTransferFile();

    // Close the socket
    closesocket(out);
    // Close down Winsock
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
    string arg;
    for (int i = 1; i < argc; ++i) {
        arg = argv[i];
        if (arg.find("--path=") != string::npos && i == 2) {
            if (arg.substr(0, 7) != "--path=") {
                cout << "Invalid syntax:" << arg << "\n";
                return false;
            }
            else
                cout << arg.substr(7, arg.length()) << "\n";
        }
        else if (arg.find("--buffer-size=") != std::string::npos && i == 3) {
            if (arg.substr(0, 14) != "--buffer-size=") {
                cout << "Invalid syntax:" << arg << "\n";
                return false;
            }
            else
                cout << arg.substr(14, arg.length()) << "\n";
        }
        else if (i == 1) {
            // Must check the valid IP Address
        }
        else {
            cout << "Invalid syntax:" << arg << "\n";
            return false;
        }
    }
    return true;
}

/*
*  Put arguments into list argument
*  @ return
*       A vector store "string" parameters
*/
vector<string> getArgs(int argc, char* argv[])
{
    vector<string> listArgs;
    string arg;
    for (int i = 1; i < argc; ++i) {
        arg = argv[i];
        switch (i) {
            case 1:
                listArgs.push_back(arg);
                break;
            case 2:
                listArgs.push_back(arg.substr(7, arg.length()));
                break;
            case 3:
                listArgs.push_back(arg.substr(14, arg.length()));
                break;
        }
    }
    return listArgs;
}


/*
*   Set IP and port for structure Receiver
*   @ param ipAddress
*       IP Address of Receiver
*/
void setReceiverSock(sockaddr_in& receiver, const char *ipAddress) {
    receiver.sin_family = AF_INET; // AF_INET = IPv4 addresses
    receiver.sin_port = htons(PORT); // Little to big endian conversion
    inet_pton(AF_INET, ipAddress, &receiver.sin_addr); // Convert from string to byte array
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
*   Get the size of a specific file
*   @ return
*       Total size
*/
int getFileSize(string pathFile) {
    ifstream file;
    file.open(pathFile, ios_base::binary);
    file.seekg(0, ios_base::end);
    int size = file.tellg();
    file.close();
    return size;
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
*   Send the basic information to Receiver such as buffer size, number of segments
*   @ param out
*       Socket
*   @ param receiver
*       structure sockaddr of Receiver
*   @ param filename
*       Path to file
*   @ param buffer_size
*       The actual buffer size (input from keyboard)
*   @ param numSegments
*       Number of segments
*/
void exchangeInformation(SOCKET out, sockaddr_in receiver, string filename, int buffer_size, int &numSegments)
{
    // Get total segment
    int totalSize = getFileSize(filename);
    // I minus 13 because I want to spend a little character for something else
    // in string data was sent to receiver
    numSegments = int(ceil(totalSize / float(buffer_size - 13)));
    string str = to_string(buffer_size) + "," + to_string(numSegments) + ",";
    int sendOk = sendto(out, str.c_str(), str.size(), 0, (sockaddr*)&receiver, sizeof(receiver));
    if (sendOk == SOCKET_ERROR)
    {
        cout << "Can't send basic information to Receiver \n" 
             << "Error code: " << WSAGetLastError() << "\n";
        exit(0);
    }
}

/*
*   Read the file in chunk parts. With every part, I process that part 
    and send to receiver and wait for response message
*   @ param out
*       Socket
*   @ param receiver
*       structure sockaddr of Receiver
*   @ param client
*       structure addr_in of the client
*   @ param filename
*       Path to file
*   @ param buffer_size
*       The actual buffer size (input from keyboard)
*/
void readFileChunk(SOCKET out, sockaddr_in receiver, sockaddr_in &client, string filename, int buffer_size)
{
    // For read file
    ifstream wif(filename);
    wif.imbue(locale(locale::empty(), new codecvt_utf8<wchar_t>));
    wstringstream wss;

    // Prepare variables for exchange Information and read file chunk
    char *buffer;
    buffer = new char[buffer_size];
    string data;
    uint32_t id = 1;
    int numSegments;

    // Send buffer_size to Recv
    exchangeInformation(out, receiver, filename, buffer_size, numSegments);

    
    vector<string> arrArgs; // Vector for store parsed data
    string recvStr; // Store string after convert from char *
    bool flag = false;

    while (!wif.eof()) {
        cout << "===== SENDING SEGMENT " << id << " =====\n";
        ZeroMemory(buffer, buffer_size); // Clear the buffer
        wif.read(buffer, buffer_size - 13); // Read a part in file following the stream

        streamsize s = wif.gcount();
        size_t bufSize = static_cast<size_t>(s);
        // Parse data to string 
        data = parseUDPdataToString(buffer, bufSize, id);

        do {
            flag = false;
            
            int sendOk = sendto(out, data.c_str(), data.size(), 0, (sockaddr*)&receiver, sizeof(receiver));
            if (sendOk == SOCKET_ERROR)
            {   
                cout << "That didn't work! " << WSAGetLastError() << endl;
                continue;
            }
            cout << "===== RECEIVE ACK " << id << " =====\n";
            /* receiv ack */
            ZeroMemory(&client, sizeof(client)); // clear the client structure
            ZeroMemory(buffer, buffer_size); // clear the receive buffer

            // wait for message
            int clientlength = sizeof(client);
            int bytesin = recvfrom(out, buffer, buffer_size, 0, (sockaddr*)&client, &clientlength);
            if (bytesin == SOCKET_ERROR) {
                cout << "ERROR receiving from Sender" 
                    << "ERROR code " << WSAGetLastError() << "\n";
                continue;
            }

            recvStr.assign(buffer, buffer + buffer_size);
            // Parse received message
            arrArgs = parseRecvData(recvStr);

            uint32_t segmentId = convertStrToUint32(arrArgs[0]);
            // If we received the response has the same ID, we'd get out of the loop 
            // Else, re-send the current trunk
            if (arrArgs[1] == "1" && segmentId == id)
                flag = true;

            arrArgs.clear();
        } while (!flag);

        // Increase ID segment
        id += 1;
    }
    delete[] buffer;
    
    cout << "============\n";
    lastSegment(out, receiver, client, buffer_size, id);
}

/*
*   Send the last segment to receiver to finish the transferring.
*   @ param out
*       Socket
*   @ param receiver
*       structure sockaddr of Receiver
*   @ param client
*       structure addr_in of the client
*   @ param buffer_size
*       The actual buffer size (input from keyboard)
*   @ param segmentID
*       ID of segment
*/
void lastSegment(SOCKET out, sockaddr_in receiver, sockaddr_in& client, int buffer_size, int segmentID) {
    string data, recvStr; // Store string after convert from char *
    bool flag = false;
    // Vector for store parsed data
    vector<string> arrArgs;
    // String to finish the transferring file
    char buf[] = "fjnjsh_tr4nf3r_fil3!";
    char* buffer = new char[buffer_size];

    // Just send a segment
    do {
        cout << "===== SENDING SEGMENT " << segmentID << " =====\n";
        /* ==== Sending segment ===*/
        data = parseUDPdataToString(buf, 21, segmentID);

        int sendOk = sendto(out, data.c_str(), data.size(), 0, (sockaddr*)&receiver, sizeof(receiver));
        if (sendOk == SOCKET_ERROR)
        {
            cout << "Can't send segment to Receiver " << WSAGetLastError() << endl;
            continue;
        }

        cout << "===== RECEIVE ACK " << segmentID << " =====\n";
        /* ==== Receive ack ===*/
        ZeroMemory(&client, sizeof(client)); // clear the client structure
        ZeroMemory(buffer, buffer_size); // clear the receive buffer

        // wait for message
        int clientlength = sizeof(client);
        int bytesin = recvfrom(out, buffer, buffer_size, 0, (sockaddr*)&client, &clientlength);
        if (bytesin == SOCKET_ERROR) {
            cout << "error receiving from sender " << WSAGetLastError() << "\n";
            continue;
        }

        recvStr.assign(buffer, buffer + buffer_size);
        arrArgs = parseRecvData(recvStr);

        uint32_t segmentIdRecv = convertStrToUint32(arrArgs[0]);
        if (arrArgs[1] == "1" && segmentIdRecv == segmentID)
            flag = true;

        arrArgs.clear();
    } while (!flag);

    delete[] buffer;
}
/*
*   Print the message
*/
void finishTransferFile() {
    cout << "\n---------------------\n"
        << "Transfer file successfully\n"
        << "---------------------\n";
}