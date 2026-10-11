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
    string indexFile = "index.idx";
    
    struct Record {
        string key;
        int value;
        bool deleted;
        long offset;
        
        Record() : value(0), deleted(false), offset(0) {}
        Record(string k, int v, bool d, long o) : key(k), value(v), deleted(d), offset(o) {}
    };
    
    // Index: key -> set of (value, offset) for efficient lookup
    unordered_map<string, set<pair<int, long>>> keyIndex;
    
    void loadIndex() {
        ifstream idx(indexFile, ios::binary);
        if (!idx.is_open()) return;
        
        string key;
        int value;
        long offset;
        bool deleted;
        
        while (idx.read(reinterpret_cast<char*>(&deleted), sizeof(bool))) {
            size_t keySize;
            idx.read(reinterpret_cast<char*>(&keySize), sizeof(size_t));
            key.resize(keySize);
            idx.read(&key[0], keySize);
            
            idx.read(reinterpret_cast<char*>(&value), sizeof(int));
            idx.read(reinterpret_cast<char*>(&offset), sizeof(long));
            
            if (!deleted) {
                keyIndex[key].insert({value, offset});
            }
        }
        idx.close();
    }
    
    void saveIndex() {
        ofstream idx(indexFile, ios::binary | ios::trunc);
        if (!idx.is_open()) return;
        
        // Read all records from data file and write to index
        ifstream data(dataFile, ios::binary);
        if (!data.is_open()) {
            idx.close();
            return;
        }
        
        data.seekg(0, ios::end);
        long fileSize = data.tellg();
        data.seekg(0, ios::beg);
        
        long offset = 0;
        while (offset < fileSize) {
            bool deleted;
            data.read(reinterpret_cast<char*>(&deleted), sizeof(bool));
            
            size_t keySize;
            data.read(reinterpret_cast<char*>(&keySize), sizeof(size_t));
            
            string key(keySize, '\0');
            data.read(&key[0], keySize);
            
            int value;
            data.read(reinterpret_cast<char*>(&value), sizeof(int));
            
            // Write to index file
            idx.write(reinterpret_cast<char*>(&deleted), sizeof(bool));
            idx.write(reinterpret_cast<char*>(&keySize), sizeof(size_t));
            idx.write(key.c_str(), keySize);
            idx.write(reinterpret_cast<char*>(&value), sizeof(int));
            idx.write(reinterpret_cast<char*>(&offset), sizeof(long));
            
            offset += sizeof(bool) + sizeof(size_t) + keySize + sizeof(int);
        }
        
        data.close();
        idx.close();
    }
    
public:
    FileStorageDB() {
        loadIndex();
    }
    
    ~FileStorageDB() {
        saveIndex();
    }
    
    void insert(const string& key, int value) {
        // Check if this key-value pair already exists
        auto it = keyIndex.find(key);
        if (it != keyIndex.end()) {
            auto& valueSet = it->second;
            for (const auto& pair : valueSet) {
                if (pair.first == value) {
                    return; // Already exists, don't insert duplicate
                }
            }
        }
        
        // Append to data file
        ofstream data(dataFile, ios::binary | ios::app);
        if (!data.is_open()) return;
        
        long offset = data.tellp();
        
        bool deleted = false;
        data.write(reinterpret_cast<char*>(&deleted), sizeof(bool));
        
        size_t keySize = key.size();
        data.write(reinterpret_cast<char*>(&keySize), sizeof(size_t));
        data.write(key.c_str(), keySize);
        
        data.write(reinterpret_cast<char*>(&value), sizeof(int));
        
        data.close();
        
        // Update index
        keyIndex[key].insert({value, offset});
    }
    
    void remove(const string& key, int value) {
        auto it = keyIndex.find(key);
        if (it == keyIndex.end()) return;
        
        auto& valueSet = it->second;
        auto valIt = valueSet.lower_bound({value, 0});
        
        // Find the exact value
        while (valIt != valueSet.end() && valIt->first == value) {
            long offset = valIt->second;
            
            // Mark as deleted in data file
            fstream data(dataFile, ios::binary | ios::in | ios::out);
            if (data.is_open()) {
                data.seekp(offset);
                bool deleted = true;
                data.write(reinterpret_cast<char*>(&deleted), sizeof(bool));
                data.close();
            }
            
            valueSet.erase(valIt);
            break;
        }
        
        if (valueSet.empty()) {
            keyIndex.erase(it);
        }
    }
    
    vector<int> find(const string& key) {
        vector<int> result;
        auto it = keyIndex.find(key);
        if (it == keyIndex.end()) {
            return result;
        }
        
        const auto& valueSet = it->second;
        for (const auto& pair : valueSet) {
            result.push_back(pair.first);
        }
        
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