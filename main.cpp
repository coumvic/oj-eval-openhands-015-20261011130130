#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_set>

using namespace std;

class FileStorageDB {
private:
    static const int NUM_BUCKETS = 256;
    
    string getBucketFile(const string& key) {
        unsigned int hash = 0;
        for (char c : key) {
            hash = hash * 31 + c;
        }
        int bucket = hash % NUM_BUCKETS;
        return "bucket_" + to_string(bucket) + ".dat";
    }
    
    struct Record {
        string key;
        int value;
        bool deleted;
        
        Record() : value(0), deleted(false) {}
        Record(string k, int v, bool d) : key(k), value(v), deleted(d) {}
    };
    
    bool existsInBucket(const string& bucketFile, const string& key, int value) {
        ifstream data(bucketFile, ios::binary);
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
    
    void insertToBucket(const string& bucketFile, const string& key, int value) {
        ofstream data(bucketFile, ios::binary | ios::app);
        if (!data.is_open()) return;
        
        bool deleted = false;
        data.write(reinterpret_cast<char*>(&deleted), sizeof(bool));
        
        size_t keySize = key.size();
        data.write(reinterpret_cast<char*>(&keySize), sizeof(size_t));
        data.write(key.c_str(), keySize);
        
        data.write(reinterpret_cast<char*>(&value), sizeof(int));
        
        data.close();
    }
    
    void removeFromBucket(const string& bucketFile, const string& key, int value) {
        fstream data(bucketFile, ios::binary | ios::in | ios::out);
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
    
    vector<int> findInBucket(const string& bucketFile, const string& key) {
        vector<int> result;
        unordered_set<int> seen; // To avoid duplicates
        
        ifstream data(bucketFile, ios::binary);
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
            
            if (!deleted && currentKey == key && seen.find(currentValue) == seen.end()) {
                result.push_back(currentValue);
                seen.insert(currentValue);
            }
            
            offset += sizeof(bool) + sizeof(size_t) + keySize + sizeof(int);
        }
        
        data.close();
        
        // Sort the results
        sort(result.begin(), result.end());
        
        return result;
    }
    
public:
    FileStorageDB() {
        // No initialization needed
    }
    
    ~FileStorageDB() {
        // No cleanup needed
    }
    
    void insert(const string& key, int value) {
        string bucketFile = getBucketFile(key);
        
        // Check if this key-value pair already exists
        if (existsInBucket(bucketFile, key, value)) {
            return; // Already exists, don't insert duplicate
        }
        
        // Insert to bucket
        insertToBucket(bucketFile, key, value);
    }
    
    void remove(const string& key, int value) {
        string bucketFile = getBucketFile(key);
        removeFromBucket(bucketFile, key, value);
    }
    
    vector<int> find(const string& key) {
        string bucketFile = getBucketFile(key);
        return findInBucket(bucketFile, key);
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