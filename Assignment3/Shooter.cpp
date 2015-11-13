#include <iostream>
#include <thread>
#include <unistd.h>
#include "Lanes.h"
#include <vector>
#include <mutex>
#include <chrono>

using namespace std;


// Debug setting
#define DEBUG_OUTPUT false
#define GRAPHIC_OUTPUT true

#define DB(fmt, ...) \
do { if (DEBUG_OUTPUT) fprintf(stderr, fmt, __VA_ARGS__); } while (0)


// Specify action with definition of: {RogueCoarse, RogueFine, RogueTM, RogueCoarse2, RogueFine2, RogueTM2,
//                                     RogueCoarseCleaner, RogueFineCleaner, RogueTMCleaner}
#define RogueCoarse


// Global variables
Lanes* lanesGallery;
int nLanes = 4;
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
    
    srand ((int)playerColor); // seed random number
    
    // Setup variables
    Color selectedLaneColor;
    Color returnColor;
    int violetLanes = 0;
    
    
    while (execute) {
        auto timeOfNextShot = chrono::system_clock::now() + chrono::milliseconds(1000/rateShotsPerSecond);
        
        // Pick random lane
        int selectedLane = rand() % nLanes;
        
        
        
#ifdef RogueCoarse
        // Check color of selected lane
        coarseLock.lock();
        selectedLaneColor = lanesGallery->Get(selectedLane); // *** lanesGallery Access ***
        DB("Player %u selected lane %d, currently %u\n", playerColor, selectedLane, selectedLaneColor);
        
        // Ensure lane is not white
        if (selectedLaneColor != white) {
            coarseLock.unlock();
            DB("Oops... lane %d is %u\n", selectedLane, selectedLaneColor);
            
            this_thread::sleep_until(timeOfNextShot);
            continue;
        }
        
        // Shoot lane
        returnColor = lanesGallery->Set(selectedLane, playerColor); // *** lanesGallery Access ***
        coarseLock.unlock();
        DB("Player %u shot lane %d\n", playerColor, selectedLane);
        
        if (returnColor != white) { // return of white indicates success
            violetLanes++;
            DB("*** Dang. We have a violet mess on our hands in lane %d ***\n", selectedLane);
        } else {
            roundLanesShot++; //the lane was shot successfully, so one more lane has been shot this round
            DB("Lane %d colored %u\n", selectedLane, playerColor);
        }
        
        // lanes are full when shotCount + violetLanes == total number of lanes
        if (roundLanesShot + violetLanes == nLanes) {
            coarseLock.lock();
            //make sure other thread did not already clean
            if (roundLanesShot + violetLanes == nLanes) {
                // Reset non-violet lanes back to white
                for (int i = 0; i < nLanes; i++){
                    if (lanesGallery->Get(i) != violet){ // *** lanesGallery Access ***
                        lanesGallery->ClearLane(i); // *** lanesGallery Access ***
                    }
                }
                roundLanesShot = 0; //new round starting now
            }
            coarseLock.unlock();
        }
        
        // Sleep to control shots to rateShotsPerSecond
        this_thread::sleep_until(timeOfNextShot);
#endif
        
        
        
        
#ifdef RogueFine
        
#endif
        
        
        
        
#ifdef RogueFine
        
#endif
        
        
        
#ifdef RogueTM
        
#endif
        
        
        
        
#ifdef RogueCoarse2
        
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
    std::vector<thread> threadsList;
    lanesGallery = new Lanes(nLanes);
    lanesGallery->Clear();
    
    threadsList.push_back(std::thread(&printer, 5));
    threadsList.push_back(std::thread(&shooterAction, 5, red));
    threadsList.push_back(std::thread(&shooterAction, 5, blue));
    
#if defined(RogueCoarseCleaner) || defined(RogueFineCleaner) || defined(RogueTMCleaner)
    threadsList.push_back(std::thread(&cleaner));
#endif
    
    // Clean up threads
    for (auto& thread : threadsList) {
        thread.join();
    }
    
    return 0;
}
