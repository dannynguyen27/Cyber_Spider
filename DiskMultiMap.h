#ifndef DISKMULTIMAP_H_
#define DISKMULTIMAP_H_

#include <string>
#include "MultiMapTuple.h"
#include "BinaryFile.h"

class DiskMultiMap
{
public:
    class Iterator
    {
    public:
        Iterator();
        // You may add additional constructors
        Iterator(bool validState, BinaryFile::Offset currentOffset, BinaryFile* diskBf);
        bool isValid() const;
        Iterator& operator++();
        MultiMapTuple operator*();
        
    private:
        // Your private member declarations will go here
        bool inValidState;
        BinaryFile::Offset nodeLocation;
        BinaryFile* bf;
    };
    
    DiskMultiMap();
    ~DiskMultiMap();
    bool createNew(const std::string& filename, unsigned int numBuckets);
    bool openExisting(const std::string& filename);
    void close();
    bool insert(const std::string& key, const std::string& value, const std::string& context);
    Iterator search(const std::string& key);
    int erase(const std::string& key, const std::string& value, const std::string& context);
    
private:
    unsigned int hashFunction(const std::string &hashme, int numBuckets);
    BinaryFile* getBinaryFile();
    // Your private member declarations will go here
    BinaryFile bf;
    
    const int MAXLENGTH = 120;
    
    struct DiskNode
    {
        char key[121];                      // Initiating Source
        char value[121];                    // What source leads to
        char context[121];                  // Which computer is linked to the source
        BinaryFile::Offset next;            // Let -1 equate to there being no next node
    };
    
    struct Bucket
    {
        BinaryFile::Offset nextNode;        // This offset represents where the actual information is located at
                                            // Let -1 represent an empty nextNode
    };
    
    struct HeaderInformation
    {
        BinaryFile::Offset nextInsert;  // Equivalent to Root Pointer
        BinaryFile::Offset deletedNodesAt;    // Tracks locations of any deleted segements in between
        int numberBuckets;
    };
    
    bool hasOpenedOrCreatedFile;
};

#endif // DISKMULTIMAP_H_