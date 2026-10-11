#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

class FileStorageDB {
private:
    string dataFile = "database.dat";
    
    struct Record {
        string key;
        int value;
        bool deleted;
        long offset;
        
        Record() : value(0), deleted(false), offset(0) {}
        Record(string k, int v, bool d, long o) : key(k), value(v), deleted(d), offset(o) {}
    };
    
    bool exists(const string& key, int value) {
        ifstream data(dataFile, ios::binary);
        if (!data.is_open()) return false;
        
        data.seekg(0, ios::end);
        long fileSize = data.tellg();
        data.seekg(0, ios::beg);
        
        long offset = 0;
        while (offset < fileSize) {
            bool deleted;
            data.read(reinterpret_cast<char*>(&deleted), sizeof(bool));
            
            size_t keySize;
            data.read(reinterpret_cast<char*>(&keySize), sizeof(size_t));
            
            string currentKey(keySize, '\0');
            data.read(&currentKey[0], keySize);
            
            int currentValue;
            data.read(reinterpret_cast<char*>(&currentValue), sizeof(int));
            
            if (!deleted && currentKey == key && currentValue == value) {
                data.close();
                return true;
            }
            
            offset += sizeof(bool) + sizeof(size_t) + keySize + sizeof(int);
        }
        
        data.close();
        return false;
    }
    
public:
    FileStorageDB() {
        // No initialization needed
    }
    
    ~FileStorageDB() {
        // No cleanup needed
    }
    
    void insert(const string& key, int value) {
        // Check if this key-value pair already exists
        if (exists(key, value)) {
            return; // Already exists, don't insert duplicate
        }
        
        // Append to data file
        ofstream data(dataFile, ios::binary | ios::app);
        if (!data.is_open()) return;
        
        bool deleted = false;
        data.write(reinterpret_cast<char*>(&deleted), sizeof(bool));
        
        size_t keySize = key.size();
        data.write(reinterpret_cast<char*>(&keySize), sizeof(size_t));
        data.write(key.c_str(), keySize);
        
        data.write(reinterpret_cast<char*>(&value), sizeof(int));
        
        data.close();
    }
    
    void remove(const string& key, int value) {
        fstream data(dataFile, ios::binary | ios::in | ios::out);
        if (!data.is_open()) return;
        
        data.seekg(0, ios::end);
        long fileSize = data.tellg();
        data.seekg(0, ios::beg);
        
        long offset = 0;
        while (offset < fileSize) {
            long currentOffset = offset;
            
            bool deleted;
            data.read(reinterpret_cast<char*>(&deleted), sizeof(bool));
            
            size_t keySize;
            data.read(reinterpret_cast<char*>(&keySize), sizeof(size_t));
            
            string currentKey(keySize, '\0');
            data.read(&currentKey[0], keySize);
            
            int currentValue;
            data.read(reinterpret_cast<char*>(&currentValue), sizeof(int));
            
            if (!deleted && currentKey == key && currentValue == value) {
                // Mark as deleted
                data.seekp(currentOffset);
                deleted = true;
                data.write(reinterpret_cast<char*>(&deleted), sizeof(bool));
                data.close();
                return;
            }
            
            offset += sizeof(bool) + sizeof(size_t) + keySize + sizeof(int);
        }
        
        data.close();
    }
    
    vector<int> find(const string& key) {
        vector<int> result;
        ifstream data(dataFile, ios::binary);
        if (!data.is_open()) return result;
        
        data.seekg(0, ios::end);
        long fileSize = data.tellg();
        data.seekg(0, ios::beg);
        
        long offset = 0;
        while (offset < fileSize) {
            bool deleted;
            data.read(reinterpret_cast<char*>(&deleted), sizeof(bool));
            
            size_t keySize;
            data.read(reinterpret_cast<char*>(&keySize), sizeof(size_t));
            
            string currentKey(keySize, '\0');
            data.read(&currentKey[0], keySize);
            
            int currentValue;
            data.read(reinterpret_cast<char*>(&currentValue), sizeof(int));
            
            if (!deleted && currentKey == key) {
                result.push_back(currentValue);
            }
            
            offset += sizeof(bool) + sizeof(size_t) + keySize + sizeof(int);
        }
        
        data.close();
        
        // Sort the results
        sort(result.begin(), result.end());
        
        return result;
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    
    FileStorageDB db;
    
    int n;
    cin >> n;
    
    for (int i = 0; i < n; i++) {
        string command;
        cin >> command;
        
        if (command == "insert") {
            string key;
            int value;
            cin >> key >> value;
            db.insert(key, value);
        } else if (command == "delete") {
            string key;
            int value;
            cin >> key >> value;
            db.remove(key, value);
        } else if (command == "find") {
            string key;
            cin >> key;
            vector<int> values = db.find(key);
            
            if (values.empty()) {
                cout << "null";
            } else {
                for (size_t j = 0; j < values.size(); j++) {
                    if (j > 0) cout << " ";
                    cout << values[j];
                }
            }
            cout << "\n";
        }
    }
    
    return 0;
}