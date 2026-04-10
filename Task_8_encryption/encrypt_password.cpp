/*
cd /workspaces/OS-VM/Task_8_encryption && gcc -c des.c -o des.o && g++ encrypt_password.cpp des.o -o encrypt
./encrypt
*/

#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>
#include <random>
#include <cstring>
using namespace std;

// wraps the C code so C++ can use it
extern "C" {
    #include "des.h" 
}

// Morris-Thomson Hashing using DES implementation
string morris_thomson_encrypt(string password, uint16_t salt) {
    key_set key_sets[17]; // array to store the 16 "sub-keys" that DES makes from one main password 
    unsigned char main_key[9] = {0}; // 8 bytes to hold the password & 9th byte to ensure there is a null-terminator (\0) at the end
    unsigned char block[8] = {0}; // the "data" being encrypted (we encrypt a block of zeros, modified by the salt)
    unsigned char output[8] = {0}; // a buffer to catch the result of each DES round

    // 1. Password as Key (DES uses 8 bytes)
    // strncpy copies 1st 8 char of the password into main_keym (DES limitation)
    strncpy((char*)main_key, password.c_str(), 8); 
    // takes main_key and creates the 16 sub-keys (key_sets)
    generate_sub_keys(main_key, key_sets);

    // 2. Apply 16-bit salt to the initial 64-bit block within its first 2 bytes
    block[0] = (salt >> 8) & 0xFF; // takes the top 8 bits of the salt and puts them in block[0]
    block[1] = salt & 0xFF; // takes the bottom 8 bits and puts them in block[1]

    // 3. Apply DES 25 times in a row (key stretching)
    for (int i = 0; i < 25; i++) {
        // the DES algorithm: takes the block (starting with the salt), encrypts it using the password (key_sets), and puts the result in output
        process_message(block, output, key_sets, ENCRYPTION_MODE);
        // Feed the output back into the input for the next round
        memcpy(block, output, 8);
    }

    // Convert result to Hex for the report
    stringstream ss;
    for(int i = 0; i < 8; i++) {
        // hex converts decimal numbers to base 16 
        // & setfill ensures the hash lengths are consistent to 16 char (e.g. 5 -> 05)
        // & (int)output converts the raw byte to an integer so the stream can format it as a readable hex string
        ss << hex << setw(2) << setfill('0') << (int)output[i];
    }
    return ss.str();
}

// Checks a user-entered password 
bool isPasswordValid(string inputPassword, string storedHash, uint16_t salt) {
    // 1. Re-run the encryption process using the input and the ORIGINAL salt
    string computedHash = morris_thomson_encrypt(inputPassword, salt);
    
    // 2. Compare the newly computed hash with the one we have on file
    return (computedHash == storedHash);
}

// Generate a list of encrypted passwords 
void displayEncryptedPasswordList(const vector<string>& passwords) {

    // Setup Random Number Generator
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<uint16_t> dist(0, 65535);

    // Print Table Header
    cout << left << setw(5)  << "No."
              << setw(15) << "Plaintext" 
              << setw(12) << "Salt" 
              << "Final Encrypted Hash" << endl;
    cout << string(80, '-') << endl;

    for (size_t i = 0; i < passwords.size(); ++i) {
        uint16_t randomSalt = dist(gen);
        
        // Use our Morris-Thomson function to generate the passwords
        string encryptedHash = morris_thomson_encrypt(passwords[i], randomSalt);
        
        // print the results in the table
        cout << left << setw(5)  << (i + 1)
                  << setw(15) << passwords[i] 
                  << "0x" << setw(6) << hex << uppercase << randomSalt 
                  << nouppercase << encryptedHash << endl;
    }
    cout << endl;
}

int main() {
    vector<string> passwordList = {
        "Admin123", "UniSem!", "SecurePass", "Kernel_99", "RootAccess",
        "Biolock1", "C++_Dev",  "UnixSystem", "Data_Sci",  "XGBoost92"
    };

    cout << "--- Morris-Thomson Password Generation (10 Samples) ---" << endl;
    displayEncryptedPasswordList(passwordList);

    // RESET TO DECIMAL MODE
    cout << dec; 
    cout << "--- Validation System Check ---" << endl;
    
    string pwd = "UniSem!"; 
    uint16_t salt = 0xABCD;
    
    string stored = morris_thomson_encrypt(pwd, salt);
    string attempt = morris_thomson_encrypt("UniSem!", salt);

    if (stored == attempt) {
        cout << "Test 1 (Correct Password): ACCESS GRANTED" << endl;
    } else {
        cout << "Test 1: FAILED - Strings do not match!" << endl;
    }

    if (!isPasswordValid("wrongPass123", stored, salt)) {
        cout << "Test 2 (Wrong Password): ACCESS DENIED" << endl;
    }

    return 0;
}