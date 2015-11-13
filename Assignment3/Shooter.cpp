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
int nLanes = 4;
int roundLanesShot = 0; //number of lanes shot (blue or red)
int roundsTotal; //total number of rounds to run
int roundsCount; //rounds executed so far
chrono::time_point<chrono::system_clock> roundStartTime;
mutex coarseLock;

void shooterAction(int rateShotsPerSecond, Color playerColor) {
    
    /*
     *  Needs synchronization. Between Red and Blue shooters.
     *  Choose a random lane and shoot.
     *  Rate: Choose a random lane every 1/rate s.
     *  PlayerColor : Red/Blue.
     */
    // start time for first time (setting in both threads should not make much of a difference)
		roundStartTime = chrono::system_clock::now();
    srand ((int)playerColor); // seed random number
    
    // Setup variables
    Color selectedLaneColor;
    Color returnColor;
    int violetLanes = 0;
    
    
    while (roundsCount<roundsTotal) {
        auto timeOfNextShot = chrono::system_clock::now() + chrono::milliseconds(1000/rateShotsPerSecond);
        
        // Pick random lane
        int selectedLane = rand() % nLanes;
        
        
        
#ifdef RogueCoarse
        // Check color of selected lane
        coarseLock.lock();
			
				//if the other thread cleaned up the final round, then stop looping
				if (roundsCount >= roundsTotal){
					break;
				}

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
        if (roundLanesShot + violetLanes >= nLanes) {
            coarseLock.lock();
            //make sure other thread did not already clean
            if (roundLanesShot + violetLanes >= nLanes) {
							
								// Get end time of round
								auto roundEndTime = chrono::system_clock::now();
								
                // Count red and blue lanes (and violet lanes to be safe)
								int redCount = 0;
								int blueCount = 0;
								int violetCount = 0;
								Color laneColor;
                for (int i = 0; i < nLanes; i++){
										laneColor = lanesGallery->Get(i); // *** lanesGallery Access ***
                    if (laneColor == blue){ 
                        blueCount++;
                    } else if (laneColor == red){
												redCount++;
										} else if (laneColor == violet){
												violetCount++;
										} else {
												std::cerr << "error: white lane being cleared" << endl;
										}
                }
								//calculate red and blue shot rates
								chrono::duration<double> roundDuration = roundEndTime-roundStartTime;
								double redShotRate = ((double) redCount)/roundDuration.count();
								double blueShotRate = ((double) blueCount)/roundDuration.count();

								//print out Gallery and red and blue shot rates
								std::cout << endl << "Round " << roundsCount+1 <<" complete" << endl;
								lanesGallery->Print();
								std::cout << "Red Shot Rate: " << redShotRate << " shots/second" << endl;
								std::cout << "Blue Shot Rate: " << blueShotRate << " shots/second" << endl;
		
								if (violetCount != 0){
									std::cout << "There were " << violetCount << "violet lanes" << endl;
								}

								//clear lanes
								lanesGallery->Clear();

								//Start new round
								roundStartTime = chrono::system_clock::now();
                roundLanesShot = 0; 
								roundsCount++;
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
      redRate = atoll(optarg);
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
