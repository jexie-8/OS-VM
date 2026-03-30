#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <map>
#include <iomanip> 
using namespace std;
using namespace filesystem;

int main(int argc, char* argv[]) {
    // handle arguments (must have atleast 2 arguments for the executable and the directory path)
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <directory_path> [bin_width]" << endl;
        return 1;
    }

    string path = argv[1]; // store directory path 
    // convert string to long long & default to 1024 if arg not provided
    long long bin_width = (argc > 2) ? stoll(argv[2]) : 1024; 

    // open output file and check that it properly was created
    ofstream outFile("outputfile.csv");
    if (!outFile.is_open()) {
        cerr << "Error: Could not create output file." << endl;
        return 1;
    }

    // this map stores: key: [Bin Start Point] -> value: [Number of Files]
    map<long long, int> histogram;
    int total_files = 0;

    cout << "Scanning directory: " << path << " with bin width " << bin_width << endl;

    // crawl the filesystem recursivley using a built-in filesystem command
    try {
        for (const auto& entry : recursive_directory_iterator(path)) { 
            if (is_regular_file(entry.status())) { // skip any non-file directories

                 // record file size and record in output file
                long long size = file_size(entry);
                outFile << size << "\n";

                // calculate which bin this file belongs to
                // formula: (size / bin_width) * bin_width gives the start of the range
                long long bin_start = (size / bin_width) * bin_width;
                histogram[bin_start]++;
                total_files++;
            }
        }
    // prevents the program from crashing if it hits a folder with restricted permissions
    } catch (const filesystem_error& e) {
        cerr << "Error accessing path: " << e.what() << endl;
    }

    outFile.close();

    // print the histogram table to terminal
    cout << setfill('-') << setw(45) << "" << endl;
    cout << setfill(' ');
    cout << left << setw(25) << "File Size Range (Bytes)" << " | " << "File Count" << endl;
    cout << setfill('-') << setw(45) << "" << endl;
    cout << setfill(' ');

    for (auto const& [bin_start, count] : histogram) {
        string range = to_string(bin_start) + " - " + to_string(bin_start + bin_width - 1);
        cout << left << setw(25) << range << " | " << count << endl;
    }

    cout << setfill('-') << setw(45) << "" << endl;
    cout << "Total Files Found: " << total_files << endl;
    cout << "File sizes saved to outputfile.csv" << endl;

    return 0;
}