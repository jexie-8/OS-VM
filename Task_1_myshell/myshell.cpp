// cd /workspaces/OS-VM/Task_1_myshell
// g++ myshell.cpp -o myshell
// ./myshell commands.txt

#include <iostream>
#include <limits>
#include <string>
#include <vector>
#include <sstream>
#include <unistd.h>     // chdir(), fork(), execvp(), dup2(), close(), isatty
#include <sys/wait.h>   // waitpid()
#include <dirent.h>     // opendir(), readdir(), closedir()
#include <fstream>      // file reading
#include <cstring>      // strerror()
#include <cstdlib>      // exit()
#include <fcntl.h>   // open()
using namespace std;
extern char **environ;  // global variable - external variable of type char** (points to array of char*)

// helper function: split the input into list of words/ strings so shell can interpret commands
vector<string> interpreter(const string &input) {
    vector<string> words;    // the list of words that will store the return variable
    stringstream ss(input);  // ss takes the input as if it is cin, allows flexibility, skips white space
    string tempword;         // temporary variable to store current word from input
    while (ss >> tempword) {        // >> operator reads word by word (or space separated)
        words.push_back(tempword);  // push the word being read onto the vector 
    }
    return words;
}

// helper function: reap all zombie cases caused by external programs wihtout blocking
// child finished, parent didnt wait, so child is finished but exists in the process table
void reapZombies() {
    int status;  // child status
    pid_t pid;   // child pid
    // waits for any child process that didnt exit, WNOHANG prevents blocking
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        cout << "Background process " << pid << " finished" << endl;
    }
}

// 1.1. cd command
void cd(const vector<string> &arg_str) {
    char cwd[1024]; // to store the path (max length 1024)

    // no directory name was provided,
    // if cwd is found using getcwd func, print it, else print error
    if (arg_str.size() < 2) { 
        if (getcwd(cwd, sizeof(cwd)) != nullptr) {
            cout << cwd  << endl; 
        } else {
            perror("getcwd failed");
        }
        return;
    }

    // change the second argument (args[1]) to c string so it can be passed to the change directory function
    // if its 0: success, if its non 0: fail -> error
    if (chdir(arg_str[1].c_str()) != 0) {
        perror("cd failed");
        return;
    }

    // change the pwd env variable using setenv if getcwd succeeds -- otherwise error
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        setenv("PWD", cwd, 1);  // overwrite (1) pwd using cwd 
    } else {
        perror("getcwd failed");
    }
}

// 1.2 dir command 
void directory(const vector<string> &arg_str) {
    string pathlookup = ".";  // '.' is the cwd by default incase no dir was entered
   
    if (arg_str.size() >= 2) pathlookup = arg_str[1]; // change the path to lookup to folder (args[1]) entered by user

    DIR *dir = opendir(pathlookup.c_str()); // open the user input file into dir (using DIR* pointer)
    if (dir == nullptr) {     // error occured return fail
        perror("dir failed");
        return;
    }

    // dirent is a struct used to define 1 dir entry (stores name, type, etc.)
    // cwf is the 'current working file' 
    struct dirent *cwf;
    // while theres a cwf, output the cwf name
    while ((cwf = readdir(dir)) != nullptr) {
        cout << cwf->d_name << "  ";
    }
    cout << endl;
    closedir(dir);
}

// 1.3 environ command
void env_var() {
    // loop over every element in the environ array 
    // each string looks like NAME=VALUE
    for (int i = 0; environ[i] != nullptr; i++) {
        cout << environ[i] << endl;
    }
}

// 1.4: set command
void set_env(const vector<string> &arg_str) {
    if (arg_str.size() < 3) {
        // cerror (like cout) if not all arguments were given (misused command)
        cerr << "Usage: set VARIABLE VALUE" << endl;
        return;
    }

    string value; 
    // use size_t since vector.size() will return an unsigned integer
    // loop that reads the value argument, appends it to the empty string 'value', which is passed to setenv 
    for (size_t i = 2; i < arg_str.size(); i++) {
        value += arg_str[i];
        if (i != arg_str.size() - 1) 
        // add a space if this is not the last word so multiple words can be processed
            value += " ";
    }

    // set the env (setenv) from variable (args[1]) to value, allow overwrite (1)
    // change is only within the current process, not permanent
    // change std string to const char** for setenv function
    if (setenv(arg_str[1].c_str(), value.c_str(), 1) != 0) {
        // non-zero return value throws an error
        perror("set failed");
    }
}

// 1.5: echo command
// takes a reference to an output stream called out which by default uses cout
// ostream could be a file or string in memory 
void echo(const vector<string> &arg_str, ostream &out = cout) {
    for (size_t i = 1; i < arg_str.size(); i++) {
        out << arg_str[i];
        if (i != arg_str.size() - 1) out << " "; // if not last word, separate with space 
    }
    out << endl; // endl flushes (output appears) and prints new line 
}

// 1.6: help command
void help_manual() {
    string manual =
        "USER MANUAL \n"
        "cd [DIR]        Change directory or show current\n"
        "dir DIR         List contents of DIR\n"
        "environ         Show environment variables\n"
        "set VAR VALUE   Set environment variable\n"
        "echo TEXT       Display TEXT\n"
        "pause           Wait for Enter key\n"
        "help            Show this manual\n"
        "quit            Exit the shell\n"
        "Supports I/O redirection (<, >, >>)\n"
        "Supports background execution (&)\n";

    if (!isatty(STDOUT_FILENO)) {
        // isatty is a system func to check if standard output is a terminal or not
        // if the file is redirected, print the manual directly 
        cout << manual;
    } else {
        // the standard output is terminal so use 'more' filter (will display page by page)
        // pipe creates 2 file descriptors to connect 2 processes, one process for reading, the other process for writing.

        int helppipes[2];
        // negative value means pipe() failed
        if (pipe(helppipes) == -1) {
            perror("pipe failed");
            return;
        }

        // parent will write the manual to pipe, and child will read from it as if it is stdin ('more' cant read memory)
        pid_t pid = fork(); // create child process
        if (pid < 0) { 
            perror("fork failed"); // negative value means error
            return;
        }

        if (pid == 0) { // if the process id is 0, then that is the child process
            close(helppipes[1]);                 // close write end, because child will write
            dup2(helppipes[0], STDIN_FILENO);    // read from pipe as stdin
            close(helppipes[0]);                 // fd has been duplicated, no need for write end, close it
            execlp("more", "more", nullptr);     // replace child process with 'more', EOF is nullptr
            perror("exec more failed"); 
            exit(EXIT_FAILURE);
        } else {
            close(helppipes[0]);  // close read end, parent will only write
            write(helppipes[1], manual.c_str(), manual.size()); // write the manual to the pipe on write end [1]
            close(helppipes[1]);  // close write end once done
            waitpid(pid, nullptr, 0);  // wait for the child process to finish before exiting
        }
    }
}

// 1.7: pause command
void pause_command(istream &inputStream, bool interactive) {
    if (!interactive) {
        return;   // if batch mode, ignore pause completely, no need to wait for the user
    }

    // inform the user to press enter
    cout << "Shell paused. Press Enter to continue..." << endl;

    string templine;
    getline(inputStream, templine);   // store next line but do nothing until user enters
}

// Requirement 5: execute background programs
void execExternal(const vector<string> &arg_str, bool background) {
    if (arg_str.empty()) return; // nothing to execute

    //  create child process using fork() and check if it failed
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");

    // 0 = child process (will run the external program)
    } else if (pid == 0) {
        // empty vector of type char* to store the arguments so they can be passed to execvp
        vector<char*> childargs; 
        // fill the vector
        for (const string &arg : arg_str) {
            childargs.push_back(const_cast<char*>(arg.c_str()));
        }

        childargs.push_back(nullptr); // to recognize end of vector 
        execvp(childargs[0], childargs.data()); // child process replaced with the external program args 

        // error if execvp returns
        perror("exec failed");
        exit(EXIT_FAILURE);

    } else {
        // parent process to handle the next non-background command in the shell
        if (!background) {
            waitpid(pid, nullptr, 0); // wait if not background
        } else {
            cout << "Process running in background with PID: " << pid << endl; // display the background process id
        }
    }
}

// Requirement 4: handle I/O redirection
void redirectionIO(vector<string> &arg_str, int &infile, int &outfile) {
    for (size_t i = 0; i < arg_str.size(); ) {
        if (arg_str[i] == "<") {
            if (i + 1 < arg_str.size()) { // user wants to redirect input to file so check it exists 
                infile = open(arg_str[i + 1].c_str(), O_RDONLY); // open the file as read only into infile
                if (infile < 0) perror("Failed to open input file");
                arg_str.erase(arg_str.begin() + i, arg_str.begin() + i + 2); // remove < and fname from vector so cmd can see args 
                continue; // restart the loop manually (not incrementing yet so no elements are skipped)
            } else {
                // < should be removed because no file name was given and error shown
                cerr << "Error: no input file specified for '<'\n"; 
                arg_str.erase(arg_str.begin() + i); 
                continue;
            }
        } 
        else if (arg_str[i] == ">") {
            if (i + 1 < arg_str.size()) { // check if the file name exists 
                outfile = open(arg_str[i + 1].c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
                // open the file, write only, create it if it doesnt exist, overwrite it if it does
                // 0644 is permission control so users can only read and the owner can read and write
                if (outfile < 0) perror("Failed to open output file");
                arg_str.erase(arg_str.begin() + i, arg_str.begin() + i + 2); // erase arguments so elements shift
                continue; // reiterate
            } else {
                cerr << "Error: no output file specified for '>'\n";
                arg_str.erase(arg_str.begin() + i);
                continue;
            }
            // infile / outfile are used to redirect stdin and stdout 
        } 
        else if (arg_str[i] == ">>") {
            if (i + 1 < arg_str.size()) { // check file exists, >> appends instead of overwriting
                outfile = open(arg_str[i + 1].c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644); 
                // same as > except append is used instead of trunacate
                if (outfile < 0) perror("Failed to open output file");
                arg_str.erase(arg_str.begin() + i, arg_str.begin() + i + 2); // same as >
                continue;
            } else {
                cerr << "Error: no output file specified for '>>'\n";
                arg_str.erase(arg_str.begin() + i);
                continue;
            }
        }
        i++;
    }
}

// the shell (includes 1.8: quit command)
void shell(istream &inputStream, bool interactive){
    string line;
    
    while (true) { // shell runs until EOF or quit cmd
        reapZombies();

        // display cwd if interactive only 
        if (interactive) {
            char cwd[1024];
            // gets the cwd to display it and displays shell if failed
            if (getcwd(cwd, sizeof(cwd)) != nullptr) {
                cout << cwd << "> ";
            } else {
                cout << "shell> ";
            }
        }

        // read input (batchfile or user input), exits the loop if end of file reached
        if (!getline(inputStream, line)) break; 

        // split input into command and arguments
        vector<string> args = interpreter(line);
        if (args.empty()) continue; // ignore empty lines

        // check if & at the end (background execution)
        bool background = false;
        if (!args.empty() && args.back() == "&") {
            background = true;
            args.pop_back(); // remove & from line
        }

        // handle any I/O redirection (<, >, >>)
        int infile = -1, outfile = -1; // default: no redirection (file descriptors)
        redirectionIO(args, infile, outfile);

        int stdin = -1, stdout = -1;
        if (infile != -1) {
            // redirect the stdin to input file
            stdin = dup(STDIN_FILENO); // store original stdin
            dup2(infile, STDIN_FILENO); // replace stdin
            close(infile); // close infile
        }
        if (outfile != -1) {
            // redirect stdout to output file
            stdout = dup(STDOUT_FILENO); 
            // STDIN_FILENO or STDOUT.. are the file descriptors for standard input/output (keyboard by default which is 0).
            dup2(outfile, STDOUT_FILENO);
            close(outfile);
        }

        // command execution
        if (args.empty()) {
            // no commands
        } else if (args[0] == "quit") {
            break;
        } else if (args[0] == "cd") {
            cd(args); 
        } else if (args[0] == "dir") {
            directory(args); 
        } else if (args[0] == "echo") {
            echo(args); 
        }
        else if (args[0] == "environ") {
            env_var();
        }
        else if (args[0] == "set") {
            set_env(args);
        }
        else if (args[0] == "pause") {
            pause_command(inputStream, interactive);
        }
        else if (args[0] == "help") {
            help_manual();
        } else {
            execExternal(args, background); // external program
        }

        // restore original stdin/stdout after redirection
        if (stdin != -1) {
            dup2(stdin, STDIN_FILENO);
            close(stdin);
        }
        if (stdout != -1) {
            dup2(stdout, STDOUT_FILENO);
            close(stdout);
        }
    }
}

int main(int arg_num, char* arg_vec[]) {
    if (arg_num > 1) { // if number of command lines is more than 1 it is a batch file so read in batch mode
        ifstream file(arg_vec[1]); // open file for reading 
        if (!file.is_open()) {
            cerr << "Failed to open file: " << arg_vec[1] << endl;
            return 1; // error and exit with code 1
        }
        shell(file, false);
    } else {
        // interactive mode takes normal user cin  
        shell(cin, true);
    }

    cout << "Exiting shell." << endl;
    return 0;
}
