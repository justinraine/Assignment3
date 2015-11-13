#include <iostream>
#include <thread>
#include <unistd.h>
#include "Lanes.h"
#include <vector>
#include <mutex>
#include <chrono>
#include <getopt.h>

using namespace std;


// Debug setting
#define DEBUG_OUTPUT true
#define GRAPHIC_OUTPUT false

#define DB(fmt, ...) \
do { if (DEBUG_OUTPUT) fprintf(stderr, fmt, __VA_ARGS__); } while (0)



// Specify action with definition of: {RogueCoarse, RogueFine, RogueTM, RogueCoarse2, RogueFine2, RogueTM2,
//                                     RogueCoarseCleaner, RogueFineCleaner, RogueTMCleaner}
#define RogueCoarse2


// Global variables
Lanes* lanesGallery;
int nLanes = 16;
int roundLanesShot = 0; //number of lanes shot (blue or red)
bool execute = true;
mutex coarseLock;

void shooterAction(int rateShotsPerSecond, Color playerColor) {
    
    /*
     *  Needs synchronization. Between Red and Blue shooters.
     *  Choose a random lane and shoot.
     *  Rate: Choose a random lane every 1/rate s.
     *  PlayerColor : Red/Blue.
     */
    
    srand ((int)chrono::system_clock::now().time_since_epoch().count() / playerColor); // seed random number
    
    // Setup variables
    Color selectedLaneColor;
    Color selectedLaneColor2; // Surpress unsused variable warnings in single lane cases. Only compatable with gcc
    Color returnColor;
    Color returnColor2;
    //int violetLanes = 0;  *** Should never have violet lanes. Use assert statements instead
    
    
    while (execute) {
        auto timeOfNextShot = (chrono::system_clock::now() + chrono::milliseconds(1000/rateShotsPerSecond));
        
        // Pick random lane
        int selectedLane = rand() % nLanes;
        int selectedLane2;
        do {
            selectedLane2 = rand() % nLanes;
        } while (selectedLane == selectedLane2);
        
        
#ifdef RogueCoarse
        // Check color of selected lane
        coarseLock.lock();
        selectedLaneColor = lanesGallery->Get(selectedLane); // *** lanesGallery Access ***
        DB("Player %u selected lane %d, currently %u\n", playerColor, selectedLane, selectedLaneColor);
        
        // Only shoot color white lanes
        if (selectedLaneColor != white) {
            coarseLock.unlock();
            this_thread::sleep_until(timeOfNextShot);
            continue;
        }
        
        // Shoot lane
        returnColor = lanesGallery->Set(selectedLane, playerColor); // *** lanesGallery Access ***
        coarseLock.unlock();
        assert(returnColor == white); // If synchronizing correctly, will always be white
        DB("Player %u shot lane %d\n", playerColor, selectedLane);
        
        roundLanesShot++; //the lane was shot successfully, so one more lane has been shot this round
        
        // lanes are full when shotCount + violetLanes == total number of lanes
        if (roundLanesShot == nLanes) {
            coarseLock.lock();
            //make sure other thread did not already clean
            if (roundLanesShot == nLanes) {
                lanesGallery->Clear(); // Clear lanes
                roundLanesShot = 0; // new round starting now
            }
            coarseLock.unlock();
        }
        
        // Sleep to control shots to rateShotsPerSecond
        this_thread::sleep_until(timeOfNextShot);
#endif
        
        
        
        
#ifdef RogueFine
        
#endif
        
        
        
        
#ifdef RogueTM
        
#endif
        
        
        
        
#ifdef RogueCoarse2
        // Check color of selected lane
        coarseLock.lock();
        selectedLaneColor = lanesGallery->Get(selectedLane); // *** lanesGallery Access ***
        selectedLaneColor2 = lanesGallery->Get(selectedLane2); // *** lanesGallery Access ***
        DB("Player %u selected lanes %d (%u) and %d(%u)\n", playerColor, selectedLane, selectedLaneColor, selectedLane2, selectedLaneColor2);
        
        // Only shoot color white lanes
        if (selectedLaneColor != white || selectedLaneColor2 != white) {
            coarseLock.unlock();
            this_thread::sleep_until(timeOfNextShot);
            continue;
        }
        
        // Shoot lane
        returnColor = lanesGallery->Set(selectedLane, playerColor); // *** lanesGallery Access ***
        returnColor2 = lanesGallery->Set(selectedLane2, playerColor); // *** lanesGallery Access ***
        coarseLock.unlock();
        assert(returnColor == white); // If synchronizing correctly, will always be white
        assert(returnColor2 == white); // If synchronizing correctly, will always be white
        DB("Player %u shot lanes %d and %d\n", playerColor, selectedLane, selectedLane2);
        
        roundLanesShot += 2; // Two more lane has been shot this round
        
        if (roundLanesShot == nLanes) {
            coarseLock.lock();
            //make sure other thread did not already clean
            if (roundLanesShot == nLanes) {
                lanesGallery->Clear(); // Clear lanes
                roundLanesShot = 0; // new round starting now
            }
            coarseLock.unlock();
        }
        
        // Sleep to control shots to rateShotsPerSecond
        this_thread::sleep_until(timeOfNextShot);
#endif
        
        
        
        
#ifdef RogueFine2
        
#endif
        
        
        
        
#ifdef RogueTM2
        
#endif
    }
}


void cleaner() {
    
    /*
     *  Cleans up lanes. Needs to synchronize with shooter.
     *  Use a monitor
     *  Should be in action only when all lanes are shot.
     *  Once all lanes are shot. Cleaner starts up.
     *  Once cleaner starts up shooters wait for cleaner to finish.
     */
    
}


void printer(int rate) {
    
    /*
     *  NOT TRANSACTION SAFE; cannot be called inside a transaction. Possible conflict with other Txs Performs I/O
     *  Print lanes periodically rate/s.
     *  If it doesn't synchronize then possible mutual inconsistency between adjacent lanes.
     *  Not a particular concern if we don't shoot two lanes at the same time.
     *
     */
    
    while (1) {
        coarseLock.lock();
#if GRAPHIC_OUTPUT == true
        lanesGallery->Print();
#endif
        coarseLock.unlock();
        sleep(1);
        //cout << lanesGallery->Count();
    }
    
}

int main(int argc, char** argv) {
    vector<thread> threadsList;
    lanesGallery = new Lanes(nLanes);
    lanesGallery->Clear();
    
    threadsList.push_back(thread(&printer, 20));
    threadsList.push_back(thread(&shooterAction, 10, red));
    threadsList.push_back(thread(&shooterAction, 10, blue));
    
#if defined(RogueCoarseCleaner) || defined(RogueFineCleaner) || defined(RogueTMCleaner)
    threadsList.push_back(std::thread(&cleaner));
#endif
    
    // Clean up threads
    for (auto& thread : threadsList) {
        thread.join();
    }
    
    return 0;
}
