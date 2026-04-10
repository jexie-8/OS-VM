// git commit: cd /workspaces/OS-VM/Task_3_paging && git add . && git commit -m "task 3 paging final commit" && git push origin main
// hash commit: git log -1 --pretty=format:"%h"
// redirect: cd /workspaces/OS-VM/Task_3_paging
// compile: g++ pagingSystem.cpp -o pagingSystem
// run with 2 args (error): ./pagingSystem 50 inputfile.txt
// run with 3 args: ./pagingSystem 50 100 inputfile.txt > outputfile.txt
// run: ./pagingSystem 50 100 inputfile.txt 

#include <iostream>
#include <vector>
#include <fstream>
#include <cstdint>
#include <limits>
using namespace std;

struct Frame {
    int pagenum; 
    uint32_t agingCounter;  
    bool referenceBit;  // true = this page was used since the last tick    
    bool exists;
    uint64_t birth;     // timestamp tie-breaker for same counter

    Frame() : pagenum(-1), agingCounter(0), referenceBit(false), exists(false), birth(0) {}
};

// aging tick for all frames
void agingTick(vector<Frame>& frames, uint32_t topBitMask) {
    for (auto& f : frames) {
        if (!f.exists) continue; // skip page if empty
        f.agingCounter >>= 1; // shift all bits to the right by 1 position
        if (f.referenceBit) f.agingCounter |= topBitMask; // reference bit is 1 if page accessed since last tick
        f.referenceBit = false; // reset reference bit 
    }
}

// aging algorithm
int simulateAging(int numFrames, const vector<int>& references, int counterBits = 8, int tickEvery = 1) {
    
    if (counterBits <= 0 || counterBits > 32) counterBits = 8;  // prevent passing an invalid counter

    // number with a 1 in the counter’s most significant bit position
    uint32_t topBitMask = 1u << (counterBits - 1); // 1u (unsigned 1) shifted to the left (<<)

    vector<Frame> frames(numFrames);
    int faults = 0;
    uint64_t t = 0;

    // loop over all the pages in the reference list
    for (size_t i = 0; i < references.size(); i++) {
        int page = references[i];
        bool found = false;

        // check if current page is already loaded
        for (auto& f : frames) {
            // 1. if page is not empty and page in frame is page being accessed = page exists in memory
            if (f.exists && f.pagenum == page) {
                f.referenceBit = true; // mark as referenced since last tick
                found = true; // flag found as true (no need to load page or perform page fault)
                break;
            }
        }

        // 2. page does not exist in memory, should be loaded
        if (!found) {
            faults++;

            // Look for empty frame
            int emptyIndex = -1;
            for (int j = 0; j < numFrames; j++) {
                if (!frames[j].exists) { emptyIndex = j; break; }
            }

            int useIndex = emptyIndex; // which frame to load the page into = if any empty frames were found

            // evict frame if non empty found
            if (useIndex == -1) {
                // find a frame with smallest counter (tie-breaker is oldest birth)
                uint32_t minCounter = numeric_limits<uint32_t>::max(); // initialize counter to find smallest counter
                uint64_t minBirth = numeric_limits<uint64_t>::max(); // initialize counter to find largest birth 
                int evicted = 0; 

                // loop over frames to find LRU (least recently used) page 
                for (int j = 0; j < numFrames; j++) {
                    // frame = LRU if its counter is less than the current counter OR if they are the same, choose the older one
                    if (frames[j].agingCounter < minCounter || (frames[j].agingCounter == minCounter && frames[j].birth < minBirth)) {
                        // update trackers and the evicted variable to track the LRU page
                        minCounter = frames[j].agingCounter; 
                        minBirth = frames[j].birth;
                        evicted = j;
                    }
                }

                // once the loop completes, the evicted frame is the one to load the page into
                useIndex = evicted;
            }

            // Load the new page into the selected frame   
            Frame& f = frames[useIndex]; 
            f.pagenum = page; 
            // mark it as present and recently used
            f.exists = true;
            f.referenceBit = true;
            f.agingCounter = topBitMask; // initialize its aging counter
            f.birth = t++; // record its arrival time for tie-breaking
        }

        // Aging step (every tickEvery page references by checking if there is a multiple of tickEvery)
        if (((i + 1) % tickEvery) == 0) {
            agingTick(frames, topBitMask); // call function
        }
    }

    return faults;
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        cout << "Usage: ./aging_plot <min_frames> <max_frames> <page_file> [step] [counter_bits] [tick_every]\n";
        return 1;
    }

    // minimum frames is stores in first argument, and max is in second, convert them to int
    int minFrames = stoi(argv[1]); 
    int maxFrames = stoi(argv[2]);
    // take third arg as file name
    string filename = argv[3];
    // number of bits for the aging counter (default is 8)
    int counterBits = (argc >= 6) ? stoi(argv[5]) : 8;
    // step size when iterating from minFrames to maxFrames (default is 1)
    int step = (argc >= 5) ? stoi(argv[4]) : 1;
    // how often to perform an aging tick (default is 1)
    int tickEvery = (argc >= 7) ? stoi(argv[6]) : 1;

    // validation
    if (step <= 0) {
        cout << "Step must be positive.\n";
        return 1;
    }

    if (tickEvery <= 0) {
        cout << "tickEvery must be positive.\n";
        return 1;
    }

    // open file 
    ifstream file(filename);
    if (!file) { cout << "Error opening file.\n"; return 1; }

    
    vector<int> references; // Create a dynamic array to hold all the page numbers from the file
    int page;
    while (file >> page) references.push_back(page); // read (>>) and add (push_back) pages from file

    if (references.empty()) { cout << "No page references found.\n"; return 1; }

    ofstream csv("results.csv"); //open or create output file 
    if (!csv) { cout << "Failed to create results.csv\n"; return 1; }
    csv << "frames,faults_per_1000\n"; // column names

    cout << "Trace references: " << references.size() << "\n"; // number of page references read from file
    cout << "frames,faults_per_1000\n"; // column names in console

    for (int f = minFrames; f <= maxFrames; f += step) { // run minframes to maxframs increasing by 'step' each time
        int faults = simulateAging(f, references, counterBits, tickEvery); // call algorithm function and record faults
        double faultsPer1000 = (double)faults / references.size() * 1000.0; // normalize fault values 

        cout << f << "," << faultsPer1000 << "\n"; // print results to console
        csv << f << "," << faultsPer1000 << "\n"; // print results to csv
    }

    cout << "Wrote results.csv\n"; // confirm csv is complete
    return 0;
}