// cd /workspaces/OS-VM/Task_5_scheduling && g++ schedulingalgo.cpp -o scheduler && ./scheduler && uv run --with matplotlib --with pandas plotting.py
#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>
#include <iomanip>
#include <queue>
using namespace std;

// simulate a PCB (process control block)
struct PCB {
    int id; // unique id for each process
    int arrTime, burstTime, remaining; // time of arrival, time needed, and time remaining 
    int waitTime = 0; // = completion time - arrival time - burst time
};

double FCFS(vector<PCB> p) {
    // sort the vector p which contains all the process from first arrived to last arrived
    sort(p.begin(), p.end(), [](PCB a, PCB b) { return a.arrTime < b.arrTime; });
    int currTime = 0;
    double totalWT = 0;
    for (auto& prcs : p) {
        // fast forward the CPU current time to go to the next process to skip idle time
        if (currTime < prcs.arrTime) currTime = prcs.arrTime;
        // calculate how long the process was waiting for
        prcs.waitTime = currTime - prcs.arrTime;
        // update the current time to account for how long this process took
        currTime += prcs.burstTime;
        // accumulate the waiting times for the fina average calculation
        totalWT += prcs.waitTime;
    }
    return totalWT / p.size(); // final average of the waiting time
}

double SJF(vector<PCB> p) {
    // initialize variables and vector
    int n = p.size();
    int completed = 0, currentTime = 0;
    double totalWT = 0;
    vector<bool> isDone(n, false);

    while (completed < n) {
        int idx = -1;
        int minBurst = 1e9;
        for (int i = 0; i < n; i++) {
            // picks the process that has arrived, hasnt finished, and has the shortest job
            if (p[i].arrTime <= currentTime && !isDone[i] && p[i].burstTime < minBurst) {
                minBurst = p[i].burstTime; // keep track of the current shortest burst time
                idx = i; // keep track of which process it is
            }
        }
        if (idx != -1) {
            // update wait time, current time, and total waiting time as done in FCFS
            p[idx].waitTime = currentTime - p[idx].arrTime; 
            currentTime += p[idx].burstTime;
            totalWT += p[idx].waitTime;
            isDone[idx] = true; // process is done so it wont be picked again
            completed++; // while loop counter 
        } else currentTime++; // no process was found so increment the time (there was idle time)
    }
    return totalWT / n; // final average of waiting times 
}

double roundRobin(vector<PCB> p, int quantum) {
    // initialization block
    int n = p.size();
    queue<int> q;
    int currentTime = 0, completed = 0;
    double totalWT = 0;
    vector<bool> inQueue(n, false);

    // lamda function to update the queue containing processes
    // process must have arrived, still has time remaining, and isnt already in the queue
    auto updateQueue = [&](int time) {
        for (int i = 0; i < n; i++) {
            if (p[i].arrTime <= time && !inQueue[i] && p[i].remaining > 0) {
                q.push(i);
                inQueue[i] = true;
            }
        }
    };
    updateQueue(currentTime);
    while (completed < n) {
        // if there is idle time (no processes are in the queue), skip the time & update the queu
        if (q.empty()) {
            currentTime++;
            updateQueue(currentTime);
            continue;
        }
        // there is a process, it should be removed from the queue
        int i = q.front(); q.pop();
        // execute the processes for 2 or 4 ms & if 1ms is left, finish the process
        int execute = min(p[i].remaining, quantum);
        // update the remaining & current time & the queue
        p[i].remaining -= execute; 
        currentTime += execute;
        updateQueue(currentTime);
        // if the process didnt finish, queue it again, otherwise calculate its wait time, 
        // accumulate the wait time, and increment the while loop counter
        if (p[i].remaining > 0) q.push(i);
        else {
            completed++;
            p[i].waitTime = currentTime - p[i].arrTime - p[i].burstTime;
            totalWT += p[i].waitTime;
        }
    }
    return totalWT / n; // average wait time calculcation for comparison in results file
}

int main() {
    // open and read file
    ifstream inFile("inputfile.txt");
    int n; inFile >> n;
    vector<PCB> processes(n);
    for (int i = 0; i < n; i++) {
        processes[i].id = i + 1;
        inFile >> processes[i].arrTime >> processes[i].burstTime;
        processes[i].remaining = processes[i].burstTime;
    }

    // run the simulation
    double fcfs = FCFS(processes);
    double sjf = SJF(processes);
    double rr = roundRobin(processes, 2);

    // save results in output file
    ofstream outFile("results.csv");
    outFile << "Algorithm,WaitTime\n";
    outFile << "FCFS," << fcfs << "\n";
    outFile << "SJF," << sjf << "\n";
    outFile << "RR," << rr << "\n";
    
    cout << "Results saved as results.csv" ;
    return 0;
}