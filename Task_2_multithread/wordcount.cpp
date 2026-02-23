// cd /workspaces/OS-VM/Task_2_multithread
// ./wordcount.cpp

#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <unordered_map>
#include <sstream>
#include <algorithm>
#include <mutex>
using namespace std;

// shared global map and mutex for merging
unordered_map<string,int> globalMap;
mutex globalMutex;

// clean file: change all uppercase to lowercase and remove punctuation
string cleanfile(const string& word) {
    string cleanword;
    // isalnum only keeps letters, tolower converts to lowercase
    for (char c : word) if (isalnum(c)) cleanword += tolower(c);
    return cleanword;
}

// count words -> print intermediate output -> merge results
void worker(const vector<string>& lines, int start, int end, int threadnum) {
    unordered_map<string,int> localMap;

    // 1. count frequency of words in each segment
    for (int i = start; i < end; ++i) {
        stringstream ss(lines[i]); // turn line into stringstream so words can be extracted
        string word;
        while (ss >> word) { // split each word (whitespace delimited)
            word = cleanfile(word); // clean word
            if (!word.empty()) {
                localMap[word]++; // increment frequency if word not empty
            }
        }
    }

    // 2. print intermediate output 
    cout << "\nThread " << threadnum << " intermediate result:\n";
    for (const auto& pair : localMap) {
        cout << pair.first << " : " << pair.second << "\n";
    }

    // 3. merge outputs into global map using mutex
    {
        lock_guard<mutex> lock(globalMutex); // lock the map so only 1 thread has access at a time, will unlock at end of scope
        for (const auto& pair : localMap) {
            globalMap[pair.first] += pair.second; // add count from local map to global map
        } // mutex automatically unlocked here
    }
}

int main() {


    string filename = "faketext.txt";
    int N = 4; // Number of threads

    // 1. open file
    ifstream file(filename);
    if (!file) {
        cerr << "Error opening file.\n";
        return 1;
    }

    // 2. read and store lines in vector 'lines'
    vector<string> lines;
    string line;
    while (getline(file, line)) {
        lines.push_back(line);
    }
    file.close();

    // 3. calculate number of lines per segment 
    int numoflines = lines.size();
    int base = numoflines / N;
    int extra = numoflines % N;

    vector<thread> threads;
    int start = 0;

    // 3. launch threads
    for (int i = 0; i < N; ++i) {
        // segment = baseline + distribute leftover lines 
        int segmentsize = base + (i < extra ? 1 : 0);  
        // calculate where this segment will end
        int end = start + segmentsize;
        threads.emplace_back(worker, cref(lines), start, end, i); // launch thread
        start = end;
    }

    // 4. wait for all threads
    for (auto& t : threads) {
        t.join();
    }

    // 5. print final result
    cout << "\nFinal result:\n";
    for (const auto& pair : globalMap) {
        cout << pair.first << " : " << pair.second << "\n";
    }

    return 0;
}