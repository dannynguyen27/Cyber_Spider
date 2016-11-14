//
//  IntelWeb.cpp
//  Project 4
//
//  Created by Danny Nguyen on 3/8/16.
//  Copyright © 2016 Danny Nguyen. All rights reserved.
//

#include <stdio.h>
#include "IntelWeb.h"
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <set>

using namespace std;

// Compares two InteractionTuples
bool operator<(const InteractionTuple& t1, const InteractionTuple& t2)
{
    if (t1.context < t2.context)
        return true;
    else if (t1.context == t2.context)
    {
        if (t1.from  < t2.from)
            return true;
        else if (t1.from == t2.from)
        {
            if (t1.to < t2.to)
            {
                return true;
            }
            else if (t1.to == t2.to)
            {
                // Both entities are the same
                return false;
            }
            else
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

IntelWeb::IntelWeb()
{
    
}

IntelWeb::~IntelWeb()
{
    myMap.close();
    myMap_reverse.close();
}

bool IntelWeb::createNew(const std::string& filePrefix, unsigned int maxDataItems)
{
    close();
    
    int totalNumberBuckets = maxDataItems/0.75;
    
    myMap_reverse.createNew(filePrefix + "_Reverse", totalNumberBuckets);
    return myMap.createNew(filePrefix, totalNumberBuckets);
}

bool IntelWeb::openExisting(const std::string& filePrefix)
{
    close();
    myMap_reverse.openExisting(filePrefix + "_Reverse");
    return myMap.openExisting(filePrefix);
}

void IntelWeb::close()
{
    myMap.close();
}

bool IntelWeb::ingest(const std::string& telemetryFile)
{
    // Open the file for input
    ifstream inf(telemetryFile);
		  // Test for failure to open
    if ( ! inf)
    {
        cout << "Cannot open " << telemetryFile << " file!" << endl;
        return false;
    }
    
    // Read each line.  The return value of getline is treated
	// as true if a line was read, false otherwise (e.g., because
	// the end of the file was reached).
    string line;
    while (getline(inf, line))
    {
        // To extract the information from the line, we'll
        // create an input stringstream from it, which acts
        // like a source of input for operator>>
        istringstream iss(line);

        // This will hold the relative values of each line
        string key;
        string value;
        string context;
        
        // The return value of operator>> acts like false
        // if we can't extract a word followed by a number
        if ( ! (iss >> context >> key >> value) )
        {
            cout << "Ignoring badly-formatted input line: " << line << endl;
            continue;
        }
        // If we want to be sure there are no other non-whitespace
        // characters on the line, we can try to continue reading
        // from the stringstream; if it succeeds, extra stuff
        // is after the double.
        char dummy;
        if (iss >> dummy) // succeeds if there a non-whitespace char
            cout << "Ignoring extra data in line: " << line << endl;
        
        // Add data to DiskMultiMaps
        myMap.insert(key, value, context);
        myMap_reverse.insert(value, key, context);
    }
    return true;
}

unsigned int IntelWeb::crawl(const std::vector<std::string>& indicators, unsigned int minPrevalenceToBeGood, std::vector<std::string>& badEntitiesFound, std::vector<InteractionTuple>& badInteractions)
{
    int numberMalicious = 0;

    // Removes any and all information that may have been in badEntities beforehand
    badEntitiesFound.clear();
    
    // Adds all known bad entities to badEntitiesFound if they are within the telemetry logs
    for (int i = 0; i < indicators.size(); i++)
    {
        DiskMultiMap::Iterator itr = myMap.search(indicators[i]);
        DiskMultiMap::Iterator itr_reverse = myMap_reverse.search(indicators[i]);
        
        if (itr.isValid() || itr_reverse.isValid())
        {
            // Element already exists in badEntitiesFound
            if (std::binary_search(badEntitiesFound.begin(), badEntitiesFound.end(), indicators[i]))
            {
                continue;
            }
            numberMalicious++;
            badEntitiesFound.push_back(indicators[i]);
            std::sort(badEntitiesFound.begin(), badEntitiesFound.end());
        }
    }
    
    for (int i = 0; i < badEntitiesFound.size(); i++)
    {
        // here badEntitiesFound[i] represents the key
        DiskMultiMap::Iterator itr = myMap.search(badEntitiesFound[i]);
        
        while (itr.isValid())
        {
            MultiMapTuple temp = *itr;
            string possibleBadEntity = temp.value;
            
            // Item already exists in badEntittiesFound
            if (std::find(badEntitiesFound.begin(), badEntitiesFound.end(), possibleBadEntity) != badEntitiesFound.end())
            {
                ++itr;
                continue;
            }
            
            // TODO: find the prevalence of the item we might add in
            int prevalenceCount = 0;
            DiskMultiMap::Iterator tempItr = myMap.search(possibleBadEntity);
            
            while (tempItr.isValid())
            {
                prevalenceCount++;
                ++tempItr;
            }
            
            tempItr = myMap_reverse.search(possibleBadEntity);
            
            while (tempItr.isValid())
            {
                prevalenceCount++;
                ++tempItr;
            }
            
            // possibleBadEntity has not appeared often enough to be considered good
            if (prevalenceCount < minPrevalenceToBeGood)
            {
                numberMalicious++;
                badEntitiesFound.push_back(possibleBadEntity);
            }
            ++itr;
        }
        
        DiskMultiMap::Iterator itr_reverse = myMap_reverse.search(badEntitiesFound[i]);
        
        while (itr_reverse.isValid())
        {
            MultiMapTuple temp = *itr_reverse;
            string possibleBadEntity = temp.value;
            
            // Item already exists in badEntittiesFound
            if (std::find(badEntitiesFound.begin(), badEntitiesFound.end(), possibleBadEntity) != badEntitiesFound.end())
            {
                ++itr_reverse;
                continue;
            }
            
            // TODO: find the prevalence of the item we might add in
            int prevalenceCount = 0;
            DiskMultiMap::Iterator tempItr = myMap.search(possibleBadEntity);
            
            while (tempItr.isValid())
            {
                prevalenceCount++;
                if (prevalenceCount > minPrevalenceToBeGood)
                    break;
                ++tempItr;
            }
            
            tempItr = myMap_reverse.search(possibleBadEntity);
            
            while (tempItr.isValid())
            {
                prevalenceCount++;
                if (prevalenceCount > minPrevalenceToBeGood)
                    break;
                ++tempItr;
            }
            
            // possibleBadEntity has not appeared often enough to be considered good
            if (prevalenceCount < minPrevalenceToBeGood)
            {
                numberMalicious++;
                badEntitiesFound.push_back(possibleBadEntity);
            }
            ++itr_reverse;
        }
    }
    
    // TODO : What about multiple copies of interactions
    
    // Sorts the badEntitiesFound vector
    std::sort(badEntitiesFound.begin(), badEntitiesFound.end());
    
    set<InteractionTuple> setOfInteractions;
    
    for (int i = 0; i < badEntitiesFound.size(); i++)
    {
        DiskMultiMap::Iterator interactionsItr = myMap.search(badEntitiesFound[i]);
        
        while (interactionsItr.isValid())
        {
            MultiMapTuple temp = *interactionsItr;
            
            string from = temp.key;
            string to = temp.value;
            string context = temp.context;
            
            InteractionTuple badInteraction(from, to, context);
            
            setOfInteractions.insert(badInteraction);
            
            ++interactionsItr;
        }
        
        // Now account for reverse interactions
        DiskMultiMap::Iterator reverseInteractions = myMap_reverse.search(badEntitiesFound[i]);
        
        while (reverseInteractions.isValid())
        {
            MultiMapTuple tempReverse = *reverseInteractions;
            
            // Rememeber that in myMap_reverse, keys and values are switched
            string fromReverse = tempReverse.value;
            string toReverse = tempReverse.key;
            string contextReverse = tempReverse.context;
            
            InteractionTuple badInteractionReverse(fromReverse, toReverse, contextReverse);
            
            setOfInteractions.insert(badInteractionReverse);
            ++reverseInteractions;
        }   
    }
    
    for (set<InteractionTuple>::iterator itr = setOfInteractions.begin(); itr != setOfInteractions.end(); itr++)
    {
        badInteractions.push_back(*itr);
    }
    return numberMalicious;
}

bool IntelWeb::purge(const std::string& entity)
{
    bool hasRemovedAtLeastOne = false;
    // Goal: For every element in myMap that contains entity, we will remove the corresponding value in myMap_Reverse
    DiskMultiMap::Iterator myMapItr;
    DiskMultiMap::Iterator reverseItr;
    
    myMapItr = myMap.search(entity);
    
    while (myMapItr.isValid())
    {
        hasRemovedAtLeastOne = true;
        MultiMapTuple temp = *myMapItr;
        ++myMapItr;
        
        myMap.erase(temp.key, temp.value, temp.context);
        myMap_reverse.erase(temp.value, temp.key, temp.context);
    }
    
    reverseItr = myMap_reverse.search(entity);
    
    // Remember that myMap_reverse contains the opposite information of myMap.reverse
    while (reverseItr.isValid())
    {
        hasRemovedAtLeastOne = true;
        MultiMapTuple temp = *reverseItr;
        ++reverseItr;
        
        myMap.erase(temp.value, temp.key, temp.context);
        myMap_reverse.erase(temp.key, temp.value, temp.context);
        
    }
    return hasRemovedAtLeastOne;
}