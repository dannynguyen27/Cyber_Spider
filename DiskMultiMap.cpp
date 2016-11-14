//
//  DiskMultiMap.cpp
//  Project 4
//
//  Created by Danny Nguyen on 3/5/16.
//  Copyright © 2016 Danny Nguyen. All rights reserved.
//

#include <stdio.h>
#include "DiskMultiMap.h"
#include "MultiMapTuple.h"
#include "BinaryFile.h"
#include <string>
#include <cstring>
#include <functional>

///////////////////////////////////////////////////////////////////////////
//  Iterator implementation
///////////////////////////////////////////////////////////////////////////
DiskMultiMap::Iterator::Iterator()
{
    inValidState = false;
    nodeLocation = -1;
}

DiskMultiMap::Iterator::Iterator(bool validState, BinaryFile::Offset currentLocation, BinaryFile* diskBf)
{
    inValidState = validState;
    nodeLocation = currentLocation;
    bf = diskBf;
}

bool DiskMultiMap::Iterator::isValid() const
{
    return inValidState;
}

// TODO: Fix this
DiskMultiMap::Iterator& DiskMultiMap::Iterator::operator++()
{
    if (!inValidState)
        return *this;
    
    // It is in a valid state, which means it currently "points" to a certain location that is valid
    BinaryFile::Offset copyNodeLocation = nodeLocation;
    
    DiskNode currentNode;
    bf->read(currentNode, copyNodeLocation);
    
    char temp[121];
    strcpy(temp, currentNode.key);
    
    // Points to the value after currentNode
    BinaryFile::Offset nextLocation = currentNode.next;
    
    while (nextLocation != -1)
    {
        DiskNode nextNode;
        bf->read(nextNode, nextLocation);
        
        // Found the next instance
        if (strcmp(temp, nextNode.key) == 0)
        {
            nodeLocation = nextLocation;
            return *this;
        }
        else
        {
            nextLocation = nextNode.next;
        }
        
    }
    
    // Hit the end of the linked list
    inValidState = false;
    return *this;
}

// TODO: Read up on caching
MultiMapTuple DiskMultiMap::Iterator::operator*()
{
    MultiMapTuple contents;
    
    DiskNode temp;
    // Grabs the contents at this node
    bf->read(temp, nodeLocation);

    contents.key = temp.key;
    contents.value = temp.value;
    contents.context = temp.context;
    
    return contents;
}

///////////////////////////////////////////////////////////////////////////
//  DiskMultiMap implementation
///////////////////////////////////////////////////////////////////////////
bool DiskMultiMap::createNew(const std::string& filename, unsigned int numBuckets)
{
    if (hasOpenedOrCreatedFile)
        bf.close();
    
    bool success = bf.createNew(filename);
    
    if (!success)
        return false;
    
    HeaderInformation head;
    head.numberBuckets = numBuckets;
    head.deletedNodesAt = -1;           // There are no deleted nodes in the first place;
    head.nextInsert = sizeof(HeaderInformation) + numBuckets * sizeof(Bucket);  // Stores the position of where we can next add an item
    
    success = bf.write(head, 0);        // Header information exists at the very beginning of the file
    
    if (!success)
    {
        return false;
    }
    
    for (int i = 0; i < numBuckets; i++)
    {
        Bucket temp;
        temp.nextNode = -1;
        bf.write(temp, sizeof(HeaderInformation) + sizeof(Bucket) * i);
    }
    
    hasOpenedOrCreatedFile = true;
    return true;
}

bool DiskMultiMap::openExisting(const std::string& filename)
{
    if (hasOpenedOrCreatedFile)
        bf.close();
    
    hasOpenedOrCreatedFile = true;
    
    return bf.openExisting(filename);
}

void DiskMultiMap::close()
{
    if (!bf.isOpen())
        return;
    else
        bf.close();
}

DiskMultiMap::~DiskMultiMap()
{
    close();
}

DiskMultiMap::DiskMultiMap()
{
    hasOpenedOrCreatedFile = false;
}

unsigned int DiskMultiMap::hashFunction(const std::string& hashMe, int numBuckets)
{
    std::hash<std::string> str_hash;
    unsigned int hashValue = static_cast<unsigned int>(str_hash(hashMe));
    unsigned int bucket = hashValue % numBuckets;
    return bucket;
}

int DiskMultiMap::erase(const std::string& key, const std::string& value, const std::string& context)
{
    // Tracks the number of items removed
    int removed = 0;
    
    // Converts the three values into cstrings
    char keyCString[120];
    strcpy(keyCString, key.c_str());
    char valueCString[120];
    strcpy(valueCString, value.c_str());
    char contextCString[120];
    strcpy(contextCString, context.c_str());
    
    // Finds the number of buckets in this file
    HeaderInformation header;
    bf.read(header, 0);
    int numBuckets = header.numberBuckets;
    
    // Finds appropriate bucket number
    int bucketNumber = hashFunction(key, numBuckets);
    
    BinaryFile::Offset correctBucketOffset = sizeof(HeaderInformation) + bucketNumber * sizeof(Bucket);
    
    Bucket bucketToEraseFrom;
    bf.read(bucketToEraseFrom, correctBucketOffset);
    
    BinaryFile::Offset nextNodeInBucket = bucketToEraseFrom.nextNode;
    
    // No nodes to examine
    if (nextNodeInBucket == -1)
    {
        return 0;
    }
    
    else
    {
        // headNode keeps track of the very first node in the bucket
        DiskNode headNode;
        bf.read(headNode, nextNodeInBucket);
        
        // First nodes are the ones we want to delete
        while (nextNodeInBucket != -1 && strcmp(headNode.key, keyCString) == 0 && strcmp(headNode.value, valueCString) == 0 && strcmp(headNode.context, contextCString) == 0)
        {
            // Will need to update the deletedNodesAt portion
            HeaderInformation header;
            bf.read(header, 0);
            
            // Saves the next node's position for later so that we can modify headNode's contents now
            BinaryFile::Offset nodeAfterNodeToBeDeleted = headNode.next;
            headNode.next = header.deletedNodesAt;
            
            // Note: nextNodeInBucket is currently at the offset where headNode is located
            bf.write(headNode, nextNodeInBucket);
            
            // Updates where the last deleted item is
            header.deletedNodesAt = nextNodeInBucket;
            bf.write(header, 0);
            
            // Forgets the current head node
            bucketToEraseFrom.nextNode = nodeAfterNodeToBeDeleted;
            nextNodeInBucket = nodeAfterNodeToBeDeleted;
            
            // Updates the bucket's information
            // Note: In the event that there are no nodes remaining, the offset in the bucket will be set to -1
            bf.write(bucketToEraseFrom, correctBucketOffset);
            
            if (nextNodeInBucket != - 1)
                bf.read(headNode, nextNodeInBucket);
            
            removed++;
        }
        
        // In the middle of the list. Current Node is guaranteed to not be the entry
        while (nextNodeInBucket != -1)
        {
            DiskNode currentNode;
            bf.read(currentNode, nextNodeInBucket);
            
            BinaryFile::Offset nextNodeToProcess = currentNode.next;
            
            if (nextNodeToProcess != -1)
            {
                DiskNode secondaryNode;
                bf.read(secondaryNode, nextNodeToProcess);
                
                if (strcmp(secondaryNode.key, keyCString) == 0 && strcmp(secondaryNode.value, valueCString) == 0 && strcmp(secondaryNode.context, contextCString) == 0)
                {
                    currentNode.next = secondaryNode.next;
                    // Updates the node before
                    bf.write(currentNode, nextNodeInBucket);
                    removed++;
                }
                else
                {
                    nextNodeInBucket = nextNodeToProcess;
                }
            }
            else
            {
                nextNodeInBucket = nextNodeToProcess;
            }
            
        }
    }
    return removed;
}

bool DiskMultiMap::insert(const std::string& key, const std::string& value, const std::string& context)
{
    if (key.length() > MAXLENGTH || value.length() > MAXLENGTH || context.length() > MAXLENGTH)
        return false;
    
    HeaderInformation header;
    bf.read(header, 0);
    
    int numBuckets = header.numberBuckets;
    int nextReusable = header.deletedNodesAt;
    
    // Finds appropriate bucket number
    int bucketNumber = hashFunction(key, numBuckets);
    
    BinaryFile::Offset correctBucketOffset = sizeof(HeaderInformation) + bucketNumber * sizeof(Bucket);
    
    Bucket bucketToInsert;
    bf.read(bucketToInsert, correctBucketOffset);
    
    BinaryFile::Offset insertNext = bucketToInsert.nextNode;
    
    // This bucket hasn't been associated with any current nodes
    if (insertNext == -1)
    {
        // Reuse a previously deleted section
        if (nextReusable != -1)
        {
            DiskNode deletedNode;
            bf.read(deletedNode, nextReusable);
            // Saves the location of the next deleted item
            BinaryFile::Offset secondReusable = deletedNode.next;
            
            DiskNode newNode;
            // Copy over relevant information
            strcpy(newNode.key, key.c_str());
            strcpy(newNode.value, value.c_str());
            strcpy(newNode.context, context.c_str());
            // Shows that there is no node after this
            newNode.next = - 1;
            
            bucketToInsert.nextNode = nextReusable;
            
            // Correctly adjusts the bucket's information
            bf.write(bucketToInsert, correctBucketOffset);
            
            // Correctly adds the newNode into the reusable position
            bf.write(newNode, nextReusable);
            
            // Adjusts nextReusable so that it points to the next deleted item
            header.deletedNodesAt = secondReusable;
            bf.write(header, 0);
        }
        
        // Expand the file's size
        else
        {
            DiskNode newNode;
            // Copy over relevant information
            strcpy(newNode.key, key.c_str());
            strcpy(newNode.value, value.c_str());
            strcpy(newNode.context, context.c_str());
            // Shows that there is no node after this
            newNode.next = - 1;
            
            // Writes the node to the newest location
            bf.write(newNode, header.nextInsert);
            
            // Updates bucket information
            bucketToInsert.nextNode = header.nextInsert;
            bf.write(bucketToInsert, correctBucketOffset);
            
            // Updates header information
            header.nextInsert += sizeof(DiskNode);
            bf.write(header, 0);
        }
    }
    
    // Follow the bucket's list until you hit the last node
    else
    {
        DiskNode newNode;
        strcpy(newNode.key, key.c_str());
        strcpy(newNode.value, value.c_str());
        strcpy(newNode.context, context.c_str());
        // Shows that there is no node after this
        newNode.next = insertNext;
        
        // Can reuse some portion of the disk
        if (nextReusable != -1)
        {
            DiskNode deletedNode;
            bf.read(deletedNode, nextReusable);
            // Saves the location of the next deleted item
            BinaryFile::Offset secondReusable = deletedNode.next;
            
            // Correctly adds the newNode into the reusable position
            bf.write(newNode, nextReusable);
            
            bucketToInsert.nextNode = header.deletedNodesAt;
            bf.write(bucketToInsert, correctBucketOffset);
            
            // Adjusts nextReusable so that it points to the next deleted item
            header.deletedNodesAt = secondReusable;
            bf.write(header, 0);
        }
        
        // Expand the disk's size
        else
        {
            // Writes the node to the newest location
            bf.write(newNode, header.nextInsert);
            bucketToInsert.nextNode = header.nextInsert;
            bf.write(bucketToInsert, correctBucketOffset);
            
            // Updates header information
            header.nextInsert += sizeof(DiskNode);
            bf.write(header, 0);
        }
    }
    
    return true;
}

DiskMultiMap::Iterator DiskMultiMap::search(const std::string& key)
{
    // Default iterator - in Invalid state
    DiskMultiMap::Iterator itr;
    
    HeaderInformation header;
    bf.read(header, 0);
    int numBuckets = header.numberBuckets;
    
    // Find correct Bucket Number
    int bucketNumber = hashFunction(key, numBuckets);
    BinaryFile::Offset correctBucketOffset = sizeof(HeaderInformation) + bucketNumber * sizeof(Bucket);
    
    // Locates correct bucket to search
    Bucket bucketToSearch;
    bf.read(bucketToSearch, correctBucketOffset);
    
    // Keeps track of the current position that we're searching
    BinaryFile::Offset currentOffset = bucketToSearch.nextNode;
    
    char keyCString[121];
    strcpy(keyCString, key.c_str());
    
    DiskNode currentNode;
    while (currentOffset != -1)
    {
        bf.read(currentNode, currentOffset);
        if (strcmp(currentNode.key, keyCString) == 0)
        {
            DiskMultiMap::Iterator correctIterator(true, currentOffset, &bf);
            return correctIterator;
        }
        else
        {
            currentOffset = currentNode.next;
        }
    }
    // No correct match found. Returns an invalid iterator
    return itr;
}

BinaryFile* DiskMultiMap::getBinaryFile()
{
    return &bf;
}