#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <set>

using namespace std;

class FileStorageDB {
private:
    string dataFile = "database.dat";
    
    // Limited in-memory index: key -> set of values
    // We only store values, not offsets, to save memory
    unordered_map<string, set<int>> keyIndex;
    
    // Maximum number of keys to keep in memory index
    static const size_t MAX_KEYS_IN_MEMORY = 1000;
    
    struct Record {
        string key;
        int value;
        bool deleted;
        long offset;
        
        Record() : value(0), deleted(false), offset(0) {}
        Record(string k, int v, bool d, long o) : key(k), value(v), deleted(d), offset(o) {}
    };
    
    bool existsInFile(const string& key, int value) {
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
    
    bool existsInMemory(const string& key, int value) {
        auto it = keyIndex.find(key);
        if (it != keyIndex.end()) {
            return it->second.count(value) > 0;
        }
        return false;
    }
    
    void addToMemoryIndex(const string& key, int value) {
        if (keyIndex.size() >= MAX_KEYS_IN_MEMORY) {
            // Clear some entries to make space
            keyIndex.clear();
        }
        keyIndex[key].insert(value);
    }
    
    void removeFromMemoryIndex(const string& key, int value) {
        auto it = keyIndex.find(key);
        if (it != keyIndex.end()) {
            it->second.erase(value);
            if (it->second.empty()) {
                keyIndex.erase(it);
            }
        }
    }
    
public:
    FileStorageDB() {
        // Load some initial data into memory index
        ifstream data(dataFile, ios::binary);
        if (!data.is_open()) return;
        
        data.seekg(0, ios::end);
        long fileSize = data.tellg();
        data.seekg(0, ios::beg);
        
        long offset = 0;
        while (offset < fileSize && keyIndex.size() < MAX_KEYS_IN_MEMORY) {
            bool deleted;
            data.read(reinterpret_cast<char*>(&deleted), sizeof(bool));
            
            size_t keySize;
            data.read(reinterpret_cast<char*>(&keySize), sizeof(size_t));
            
            string key(keySize, '\0');
            data.read(&key[0], keySize);
            
            int value;
            data.read(reinterpret_cast<char*>(&value), sizeof(int));
            
            if (!deleted) {
                keyIndex[key].insert(value);
            }
            
            offset += sizeof(bool) + sizeof(size_t) + keySize + sizeof(int);
        }
        
        data.close();
    }
    
    ~FileStorageDB() {
        // No cleanup needed
    }
    
    void insert(const string& key, int value) {
        // Check if this key-value pair already exists
        if (existsInMemory(key, value) || existsInFile(key, value)) {
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
        
        // Update memory index
        addToMemoryIndex(key, value);
    }
    
    void remove(const string& key, int value) {
        fstream data(dataFile, ios::binary | ios::in | ios::out);
        if (!data.is_open()) return;
        
        data.seekg(0, ios::end);
        long fileSize = data.tellg();
        data.seekg(0, ios::beg);
        
        long offset = 0;
        bool found = false;
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
                found = true;
                break;
            }
            
            offset += sizeof(bool) + sizeof(size_t) + keySize + sizeof(int);
        }
        
        data.close();
        
        if (found) {
            removeFromMemoryIndex(key, value);
        }
    }
    
    vector<int> find(const string& key) {
        vector<int> result;
        
        // First check memory index
        auto it = keyIndex.find(key);
        if (it != keyIndex.end()) {
            result.insert(result.end(), it->second.begin(), it->second.end());
        }
        
        // Then check file for any additional entries
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
                // Check if already in result (from memory index)
                if (std::find(result.begin(), result.end(), currentValue) == result.end()) {
                    result.push_back(currentValue);
                }
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