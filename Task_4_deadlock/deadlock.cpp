// git commit: cd /workspaces/OS-VM/Task_4_deadlock && git add . && git commit -m "task 4 initial commit" && git push origin main
// hash commit: git log -1 --pretty=format:"%h"

#include <iostream>
#include <vector>
#include <fstream>
using namespace std;

void DeadlockDetection(const string& filename) {
    // open file + exit if failed
    ifstream inFile(filename);
    if (!inFile) {
        cerr << "Error opening file" << endl;
        return;
    }

    // take inputs from file for num of processes and resources
    int numPrc, numRsrc;
    inFile >> numPrc >> numRsrc;

    // read the total number of resources from file
    vector<int> E(numRsrc);
    for (int& x : E) inFile >> x;

    vector<int> AllocationMatrix(numPrc * numRsrc); // P X R matrix where(i, j) is the quantity of resource j currently held by process i.
    vector<int> RsrcSum(numRsrc, 0); // total units of each resource type currently held by all processes.
    vector<bool> finished(numPrc, false); // optimization: mark process as finished if not using any resources (could not be part of deadlock)

    // for loop to populate the vectors, loops over every process once, resets flag to false
    for (int i = 0; i < numPrc; ++i) {
        bool hasAllocation = false; 

        // loop through every resource type (j) in this process (i)
        for (int j = 0; j < numRsrc; ++j) { 
            // record at this index the quantity of (j) for (i) +  record in total sum vector + if rsrc found, update flag
            inFile >> AllocationMatrix[i * numRsrc + j]; 
            RsrcSum[j] += AllocationMatrix[i * numRsrc + j]; 
            if (AllocationMatrix[i * numRsrc + j] > 0) hasAllocation = true; 
        }

        // optimization: If process holds no resources, it's not deadlocked, will be ignored
        if (!hasAllocation) finished[i] = true; 
    }

    // reads what resources are being requested (R) by each process for it to finish
    vector<int> R(numPrc * numRsrc); 
    for (int i = 0; i < numPrc * numRsrc; ++i) inFile >> R[i];

    // calculates available resources (A) = all resources (E) - allocated resources (currentAlloc)
    vector<int> A(numRsrc);
    for (int j = 0; j < numRsrc; ++j) A[j] = E[j] - RsrcSum[j];

    // start algorithm
    bool progress = true;
    while (progress) {
        progress = false;
        for (int i = 0; i < numPrc; ++i) { // loop processes, skip if finished
            if (!finished[i]) {
                bool canProceed = true; // assume resources are available
                for (int j = 0; j < numRsrc; ++j) {
                    // compare request to available resources (if R > A, cant help process finish)
                    if (R[i * numRsrc + j] > A[j]) { 
                        canProceed = false; 
                        break;
                    }
                }

                if (canProceed) {
                    for (int j = 0; j < numRsrc; ++j) {
                        // since this process will finish, reallocate its resources to the available vector
                        A[j] += AllocationMatrix[i * numRsrc + j];
                    }
                    // update flags
                    finished[i] = true;
                    progress = true;
                }
            }
        }
    }

    // final verdict outputs (assume there is no deadlock)
    bool deadlocked = false;
    for (int i = 0; i < numPrc; ++i) {
        // search for any unfinished processes & print them
        if (!finished[i]) { 
            if (!deadlocked) cout << "Deadlocked processes: ";
            cout << i << " "; 
            deadlocked = true;
        }
    }
    if (!deadlocked) cout << "No deadlock detected.";
    cout << endl;
}

int main() {
    DeadlockDetection("inputfile.txt");
    return 0;
}