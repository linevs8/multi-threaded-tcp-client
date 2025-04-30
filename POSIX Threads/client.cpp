#include <iostream>
#include <string>
#include <vector>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>

using namespace std;

// Structure to store thread arguments
struct ThreadArgs {
    string message;
    string hostname;
    int portno;
    string* result;
};

// Thread function to handle client requests
void *clientThread(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;

    int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[1024];

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        cerr << "ERROR opening socket" << endl;
        pthread_exit(NULL);
    }

    server = gethostbyname(args->hostname.c_str());
    if (server == NULL) {
        cerr << "ERROR, no such host" << endl;
        pthread_exit(NULL);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(args->portno);

    // Connect to server
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        cerr << "ERROR connecting" << endl;
        pthread_exit(NULL);
    }

    // Send message to server
    n = write(sockfd, args->message.c_str(), args->message.length());
    if (n < 0) {
        cerr << "ERROR writing to socket" << endl;
        pthread_exit(NULL);
    }

    // Receive response from server
    bzero(buffer, 1024);
    n = read(sockfd, buffer, 1023);
    if (n < 0) {
        cerr << "ERROR reading from socket" << endl;
        pthread_exit(NULL);
    }

    *(args->result) = buffer;

    close(sockfd);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <hostname> <port_no>" << endl;
        exit(1);
    }

    string hostname = argv[1];
    int portno = atoi(argv[2]);

    vector<string> messages;
    string line;
    while (getline(cin, line)) {
        messages.push_back(line);
    }

    vector<pthread_t> threads(messages.size());
    vector<ThreadArgs> threadArgs(messages.size());
    vector<string> results(messages.size());

    // Create threads for each message
    for (size_t i = 0; i < messages.size(); ++i) {
        threadArgs[i] = {messages[i], hostname, portno, &results[i]};
        pthread_create(&threads[i], NULL, clientThread, &threadArgs[i]);
    }

    // Wait for threads to finish
    for (size_t i = 0; i < messages.size(); ++i) {
        pthread_join(threads[i], NULL);
    }

    // Print results
    for (size_t i = 0; i < messages.size(); ++i) {
        cout << "Message: " << messages[i] << endl << endl;
        cout << results[i];
    }

    return 0;
}