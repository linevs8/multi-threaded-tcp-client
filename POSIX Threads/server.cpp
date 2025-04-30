// server.cpp
#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <algorithm>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>

using namespace std;

// Structure to store symbol and its frequency
struct Symbol {
    char ch;
    int frequency;
    string code;
};

void shannonFano(int l, int h, vector<Symbol>& symbols) {
    if (l >= h) {
        return;
    }

    // Calculate total frequency
    int totalFrequency = 0;
    for (int i = l; i <= h; ++i) {
        totalFrequency += symbols[i].frequency;
    }

    // Find the partition point to split symbols into two halves
    int splitIndex = l;
    int sum1 = 0;
    int minDifference = totalFrequency;
    for (int i = l; i < h; ++i) {
        sum1 += symbols[i].frequency;
        int sum2 = totalFrequency - sum1;
        int difference = abs(sum1 - sum2);
        if (difference < minDifference) {
            minDifference = difference;
            splitIndex = i;
        }
    }

    // Assign codes based on split
    for (int i = l; i <= splitIndex; ++i) {
        symbols[i].code += "0";
    }
    for (int i = splitIndex + 1; i <= h; ++i) {
        symbols[i].code += "1";
    }

    // Recursively apply the Shannon-Fano algorithm to both halves
    shannonFano(l, splitIndex, symbols);
    shannonFano(splitIndex + 1, h, symbols);
}


// Function to encode the message
string encodeMessage(const string& message, const vector<Symbol>& alphabet) {
    string encodedMessage = "";
    for (char ch : message) {
        for (const auto& symbol : alphabet) {
            if (symbol.ch == ch) {
                encodedMessage += symbol.code;
                break;
            }
        }
    }
    return encodedMessage;
}

// Custom sorting function for symbols based on the provided rules
bool customSort(const Symbol& a, const Symbol& b) {
    // 1. Order by frequency (highest to lowest)
    if (a.frequency != b.frequency) {
        return a.frequency > b.frequency;
    }
    // 2. Spaces after letters
    if ((a.ch == ' ' && b.ch != ' ') || (b.ch == ' ' && a.ch != ' ')) {
        return b.ch == ' ';
    }
    // 3. Numbers after letters
    bool a_is_digit = isdigit(a.ch);
    bool b_is_digit = isdigit(b.ch);
    if (a_is_digit != b_is_digit) {
        return !a_is_digit;
    }
    // 4. Letters are sorted in descending order
    if (isalpha(a.ch) && isalpha(b.ch)) {
        return a.ch > b.ch;
    }
    // 5. Numbers are sorted in descending order
    if (a_is_digit && b_is_digit) {
        return a.ch > b.ch;
    }
    return false;
}

// Function to handle zombie processes
void fireman(int) {
    while (waitpid(-1, NULL, WNOHANG) > 0) {
        // Do nothing - just reaping zombie processes
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <port_no>" << endl;
        exit(1);
    }

    int portno = atoi(argv[1]);
    int sockfd, newsockfd, clilen;
    struct sockaddr_in serv_addr, cli_addr;

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        cerr << "ERROR opening socket" << endl;
        exit(1);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    // Bind socket to address
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        cerr << "ERROR on binding" << endl;
        exit(1);
    }

    // Listen for incoming connections
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    // Handle zombie processes
    signal(SIGCHLD, fireman);

    while (true) {
        // Accept a connection
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t*)&clilen);
        if (newsockfd < 0) {
            cerr << "ERROR on accept" << endl;
            exit(1);
        }

        // Fork child process
        if (fork() == 0) {
            close(sockfd);

            char buffer[1024];
            bzero(buffer, 1024);

            // Read message from client
            int n = read(newsockfd, buffer, 1023);
            if (n < 0) {
                cerr << "ERROR reading from socket" << endl;
                exit(1);
            }

            string message = buffer;

            // Create alphabet and calculate frequencies
            vector<Symbol> alphabet;
            for (char ch : message) {
                auto it = find_if(alphabet.begin(), alphabet.end(),
                                 [&ch](const Symbol& s){ return s.ch == ch; });
                if (it != alphabet.end()) {
                    it->frequency++;
                } else {
                    alphabet.push_back({ch, 1, ""});
                }
            }

            // Sort alphabet by custom rules
            sort(alphabet.begin(), alphabet.end(), customSort);

            // Generate Shannon-Fano codes
            shannonFano(0, alphabet.size() - 1, alphabet);

            // Encode the message
            string encodedMessage = encodeMessage(message, alphabet);

            // Send alphabet and encoded message to client
            string output = "Alphabet:\n";
            for (const auto& symbol : alphabet) {
                output += "Symbol: " + string(1, symbol.ch) + ", Frequency: " + to_string(symbol.frequency) +
                          ", Shannon code: " + symbol.code + "\n";
            }
            output += "\nEncoded message: " + encodedMessage + "\n";

            n = write(newsockfd, output.c_str(), output.length());
            if (n < 0) {
                cerr << "ERROR writing to socket" << endl;
                exit(1);
            }

            close(newsockfd);
            exit(0);
        } else {
            close(newsockfd);
        }
    }

    close(sockfd);
    return 0;
}
