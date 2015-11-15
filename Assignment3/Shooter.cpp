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
#define DEBUG_OUTPUT false
#define GRAPHIC_OUTPUT false

#define DB(fmt, ...) \
do { if (DEBUG_OUTPUT) fprintf(stderr, fmt, __VA_ARGS__); } while (0)



// Specify action with definition of: {RogueCoarse, RogueFine, RogueTM, RogueCoarse2, RogueFine2, RogueTM2,
//                                     RogueCoarseCleaner, RogueFineCleaner, RogueTMCleaner}


/*
 * Define statement was here. Now, choose RogueCoarse type by calling:
 *		make ROGUE=-DRogueCoarse
 * Replace RogueCoarse with the Rogue you want to run
 */


// Global variables
Lanes* lanesGallery;
int nLanes = 10;
int roundLanesShot = 0; //number of lanes shot (blue or red)
int redRoundLanesShot = 0; //for finegrain cases
int blueRoundLanesShot = 0; //for finegrain cases
int roundsTotal; //total number of rounds to run
int roundsCount; //rounds executed so far
chrono::time_point<chrono::system_clock> roundStartTime;
mutex coarseLock;
mutex * fineLock = new mutex [nLanes];
mutex fineCountLock[2]; //lock to protect counts of lanes shot by each thread. 0 for red, 1 for blue

// Function prototypes
bool unsafeSetupNextRound();

void shooterAction(int rateShotsPerSecond, Color playerColor) {
    roundStartTime = chrono::system_clock::now();
    srand ((int)chrono::system_clock::now().time_since_epoch().count() / playerColor); // seed random number
    
    // Setup variables
    Color selectedLaneColor;
    __attribute__((unused))Color selectedLaneColor2; // Surpress unsused variable warnings in single lane cases. Only compatable with gcc
    Color returnColor;
    __attribute__((unused))Color returnColor2;
    //int violetLanes = 0;  *** Should never have violet lanes. Use assert statements instead
    
    
    while (roundsCount<roundsTotal) {
        auto timeOfNextShot = chrono::system_clock::now() + chrono::milliseconds(1000/rateShotsPerSecond);
        
        // Pick random lane
        int selectedLane = rand() % nLanes;
        int selectedLane2;
        do {
            selectedLane2 = rand() % nLanes;
        } while (selectedLane == selectedLane2);
        
        
#ifdef RogueCoarse
        // Check color of selected lane
        coarseLock.lock();
        
        //if the other thread cleaned up the final round, then stop looping
        if (roundsCount >= roundsTotal){
						coarseLock.unlock();
            break;
        }
        
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
				roundLanesShot++; //the lane was shot successfully, so one more lane has been shot this round
				if (playerColor == red){
					redRoundLanesShot++;
				} else {
					blueRoundLanesShot++;
				}
        coarseLock.unlock();
        assert(returnColor == white); // If synchronizing correctly, will always be white
        DB("Player %u shot lane %d\n", playerColor, selectedLane);
        
        
        
        // lanes are full when shotCount == total number of lanes
        if (roundLanesShot == nLanes) {
            coarseLock.lock();
            unsafeSetupNextRound();
            coarseLock.unlock();
        }
        
        // Sleep to control shots to rateShotsPerSecond
        this_thread::sleep_until(timeOfNextShot);
#endif
        
        
        
        
#ifdef RogueFine
         // Check color of selected lane
        fineLock[selectedLane].lock();
			
				//if the other thread cleaned up the final round, then stop looping
				if (roundsCount >= roundsTotal){
					finelock[selectedLane].unlock();
					break;
				}

        selectedLaneColor = lanesGallery->Get(selectedLane); // *** lanesGallery Access ***
        DB("Player %u selected lane %d, currently %u\n", playerColor, selectedLane, selectedLaneColor);
        
        // Only shoot color white lanes
        if (selectedLaneColor != white) {
            fineLock[selectedLane].unlock();
            this_thread::sleep_until(timeOfNextShot);
            continue;
        }
        
        // Shoot lane
        returnColor = lanesGallery->Set(selectedLane, playerColor); // *** lanesGallery Access ***
        fineLock[selectedLane].unlock();
        assert(returnColor == white); // If synchronizing correctly, will always be white
        DB("Player %u shot lane %d\n", playerColor, selectedLane);

				if (playerColor == red) {
					fineCountLock[0].lock();
					redRoundLanesShot++; //the lane was shot successfully, so one more lane has been shot this round
					fineCountLock[0].unlock();
				} else {
					fineCountLock[1].lock();
					blueRoundLanesShot++; //the lane was shot successfully, so one more lane has been shot this round
					fineCountLock[1].unlock();
				}
        

        
        // lanes are full when shotCount + violetLanes == total number of lanes
        if ((redRoundLanesShot + blueRoundLanesShot) == nLanes) {
            for (int i=0; i < nLanes; i++){
							fineLock[i].lock();
						}
						fineCountLock[0].lock();
						fineCountLock[1].lock();

            unsafeSetupNextRound();

            for (int i=0; i < nLanes; i++){
							fineLock[i].unlock();
						}
						fineCountLock[0].unlock();
						fineCountLock[1].unlock();
        }
        
        // Sleep to control shots to rateShotsPerSecond
        this_thread::sleep_until(timeOfNextShot);
#endif
        
        
        
        
#ifdef RogueTM
        
#endif
        
        
        
        
#ifdef RogueCoarse2
        // Check color of selected lane
        coarseLock.lock();
        selectedLaneColor = lanesGallery->Get(selectedLane); // *** lanesGallery Access ***
        selectedLaneColor2 = lanesGallery->Get(selectedLane2); // *** lanesGallery Access ***
        DB("Player %u selected lanes %d (%u) and %d(%u)\n", playerColor, selectedLane, selectedLaneColor, selectedLane2, selectedLaneColor2);
        
				//if the other thread cleaned up the final round, then stop looping
        if (roundsCount >= roundsTotal){
						coarseLock.unlock();
            break;
        }

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
        if (playerColor == red){
					redRoundLanesShot += 2;
				} else {
					blueRoundLanesShot += 2;
				}

        if (roundLanesShot == nLanes) {
            coarseLock.lock();
            unsafeSetupNextRound();
            coarseLock.unlock();
        }
        
        // Sleep to control shots to rateShotsPerSecond
        this_thread::sleep_until(timeOfNextShot);
#endif
        
        
        
        
#ifdef RogueFine2
				// get locks in ascending index order to avoid deadlocking
				if (selectedLane < selectedLane2){
        	fineLock[selectedLane].lock();
					fineLock[selectedLane2].lock();
				} else {
					fineLock[selectedLane2].lock();
					fineLock[selectedLane].lock();
				}

				//if the other thread cleaned up the final round, then stop looping
				if (roundsCount >= roundsTotal){
        	fineLock[selectedLane].unlock();
					fineLock[selectedLane2].unlock();
					break;
				}

        // Check color of selected lanes
        selectedLaneColor = lanesGallery->Get(selectedLane); // *** lanesGallery Access ***
				selectedLaneColor2 = lanesGallery->Get(selectedLane2); // *** lanesGallery Access ***
        DB("Player %u selected lanes %d (%u) and %d(%u)\n", playerColor, selectedLane, selectedLaneColor, selectedLane2, selectedLaneColor2);
        
        // Only shoot color white lanes
        if (selectedLaneColor != white || selectedLaneColor2 != white) {
        		fineLock[selectedLane].unlock();
						fineLock[selectedLane2].unlock();
            this_thread::sleep_until(timeOfNextShot);
            continue;
        }
        
        // Shoot lanes
        returnColor = lanesGallery->Set(selectedLane, playerColor); // *** lanesGallery Access ***
				returnColor2 = lanesGallery->Set(selectedLane2, playerColor); // *** lanesGallery Access ***
        fineLock[selectedLane].unlock();
				fineLock[selectedLane2].unlock();
        assert(returnColor == white); // If synchronizing correctly, will always be white
        assert(returnColor2 == white); // If synchronizing correctly, will always be white
        DB("Player %u shot lanes %d and %d\n", playerColor, selectedLane, selectedLane2);

				if (playerColor == red) {
					fineCountLock[0].lock();
					redRoundLanesShot += 2; //the lane was shot successfully, so 2 more lanes has been shot this round
					fineCountLock[0].unlock();
				} else {
					fineCountLock[1].lock();
					blueRoundLanesShot += 2; //the lane was shot successfully, so 2 more lanes has been shot this round
					fineCountLock[1].unlock();
				}
        

        
        // lanes are full when shotCount == total number of lanes
        if ((redRoundLanesShot + blueRoundLanesShot) == nLanes) {
            for (int i=0; i < nLanes; i++){
							fineLock[i].lock();
						}
						fineCountLock[0].lock();
						fineCountLock[1].lock();

            unsafeSetupNextRound();

            for (int i=0; i < nLanes; i++){
							fineLock[i].unlock();
						}
						fineCountLock[0].unlock();
						fineCountLock[1].unlock();
        }
        
        // Sleep to control shots to rateShotsPerSecond
        this_thread::sleep_until(timeOfNextShot);
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
    
    //print out while shots are being fired
    while (roundsCount < roundsTotal) {
        //calculate next time to print output
        auto timeOfNextShot = chrono::system_clock::now() + chrono::milliseconds(1000/rate);
        
        coarseLock.lock();
        lanesGallery->Print();
        coarseLock.unlock();
        
        // Sleep to control shots to rateShotsPerSecond
        this_thread::sleep_until(timeOfNextShot);
        //cout << lanesGallery->Count();
    }
}


static struct option long_options[] =
{
    {"red shooter rate", required_argument, 0, 'r'},
    {"blue shooter rate", required_argument, 0, 'b'},
    {"number of rounds", required_argument, 0, 'n'},
    {0, 0, 0}
};

int main(int argc, char** argv) {
    
    // read input arguments
    int redRate;
    int blueRate;
    
    while (true) {
        
        int option_index = 0;
        int c = getopt_long_only(argc, argv, "r:b:n:",
                                 long_options, &option_index);
        
        /* Detect the end of the options. */
        if (c == -1)
            break;
        
        switch (c) {
            case 0:
                /* If this option set a flag, do nothing else now. */
                break;
                
            case 'r':
                redRate = (int)atoll(optarg);
                break;
                
            case 'b':
                blueRate = atoi(optarg);
                break;
                
            case 'n':
                roundsTotal = atoi(optarg);
                break;
                
            default:
                exit(1);
        }
    }
    //end of reading input arguments
    std::vector<thread> threadsList;
    lanesGallery = new Lanes(nLanes);
    lanesGallery->Clear();
    
#if GRAPHIC_OUTPUT == true
    threadsList.push_back(std::thread(&printer, redRate + blueRate));
#endif
    
    threadsList.push_back(std::thread(&shooterAction, redRate, red));
    threadsList.push_back(std::thread(&shooterAction, blueRate, blue));
    
#if defined(RogueCoarseCleaner) || defined(RogueFineCleaner) || defined(RogueTMCleaner)
    threadsList.push_back(std::thread(&cleaner));
#endif
    
    // Clean up threads
    for (auto& thread : threadsList) {
        thread.join();
    }
    
    return 0;
}


// If lanesGallery is full, this function will output the round info, clear the lanes back to white, and start the next round
// WARNING: Not threadsafe. Provide synchronization in calling function.
bool unsafeSetupNextRound() {
    //make sure other thread did not already clean
    if (roundLanesShot >= nLanes || (redRoundLanesShot + blueRoundLanesShot) >= nLanes) {
        // Get end time of round
        auto roundEndTime = chrono::system_clock::now();
        
        // Calculate red and blue shot rates
        chrono::duration<double> roundDuration = roundEndTime-roundStartTime;
        double redShotRate = ((double) redRoundLanesShot)/roundDuration.count();
        double blueShotRate = ((double) blueRoundLanesShot)/roundDuration.count();
        
        // Print lanesGallery and red and blue shot rates
        std::cout << endl << "Round " << roundsCount+1 <<" complete" << endl;
        lanesGallery->Print();
        std::cout << "Red Shot Rate: " << redShotRate << " shots/second" << endl;
        std::cout << "Blue Shot Rate: " << blueShotRate << " shots/second" << endl;
        
        //if (violetCount != 0){
        //   std::cout << "There were " << violetCount << "violet lanes" << endl;
        //}
        
        // Clear lanes
        lanesGallery->Clear();
        
        // Start new round
        roundStartTime = chrono::system_clock::now();
        roundLanesShot = 0;
				redRoundLanesShot = 0;
				blueRoundLanesShot = 0;
        roundsCount++;
        
        return true;
    } else {
        return false;
    }
}



