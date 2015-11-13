#include <iostream>
#include <thread>
#include <unistd.h>
#include "Lanes.h"
#include <vector>
#include <mutex>
#include <chrono>

using namespace std;

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
    
    // Define different locks
    //mutex coarseLock;
    
    // Setup variables
    Color selectedLaneColor;
    Color returnColor;
    //int roundShotCount = 0; // number of shots this thread has shot in the current round
    int violetLanes = 0;
    
    
    while (execute) {
        //cout << endl;
        auto timeOfNextShot = chrono::system_clock::now() + chrono::microseconds(1000000/rateShotsPerSecond);
        
        // Pick random lane
        int selectedLane = rand() % nLanes;

        
#ifdef RogueCoarse
        // Check color of selected lane
        coarseLock.lock();
        selectedLaneColor = lanesGallery->Get(selectedLane); // RACE ***
        //printf("Player %u selected lane %d, currently %u\n", playerColor, selectedLane, selectedLaneColor);
        
        // Ensure lane is not white
        if (selectedLaneColor != white) {
            coarseLock.unlock();
            //printf("Oops... lane %d is %u\n", selectedLane, selectedLaneColor);
            this_thread::sleep_until(timeOfNextShot);
            continue;
        }
        
        // Shoot lane
        returnColor = lanesGallery->Set(selectedLane, playerColor);
        coarseLock.unlock();
        
        //printf("Player %u shot lane %d\n", playerColor, selectedLane);
        
        if (returnColor != white) { // return of white indicates success
            //printf("*** Dang. We have a violet mess on our hands in lane %d ***\n", selectedLane);
            violetLanes++;
        } else {
            //printf("Lane %d colored %u\n", selectedLane, playerColor);
						roundLanesShot++; //the lane was shot successfully, so one more lane has been shot this round
        }
        
        
        // lanes are full when shotCount + violetLanes == total number of lanes
        if (roundLanesShot + violetLanes == nLanes) {
						coarseLock.lock();
						//make sure other thread did not already clean
						if (roundLanesShot + violetLanes == nLanes) {
		          // Reset non-violet lanes back to white
							for (int i = 0; i < nLanes; i++){
								if (lanesGallery->Get(i) != violet){
									lanesGallery->ClearLane(i);
								}
							}
							roundLanesShot = 0; //new round starting now
						}						
						coarseLock.unlock();
        }
        
        // Sleep to control shots to rateShotsPerSecond
        this_thread::sleep_until(timeOfNextShot);
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
       lanesGallery->Print();
       coarseLock.unlock();
       sleep(1);
       //cout << lanesGallery->Count();
   }

}


//void unsafeCleanNonVioletLanes() {
//    for (int i = 0; i < lanesGallery->Count(); i++) {
//        if (lanesGallery->Get(i) != violet) {
//            lanesGallery->Set(<#int index#>, <#Color myLaneColor#>)
//        }
//    }
//}
//}



int main(int argc, char** argv) {
    std::vector<thread> threadsList;
    lanesGallery = new Lanes(nLanes);
    lanesGallery->Clear();

#ifdef RogueCoarse
    threadsList.push_back(std::thread(&printer, 5));
    threadsList.push_back(std::thread(&shooterAction, 5, red));
    threadsList.push_back(std::thread(&shooterAction, 5, blue));
    //threadsList.push_back(std::thread(&cleaner));
#endif

    
    // Clean up threads
    for (auto& thread : threadsList) {
        thread.join();
    }

    return 0;
}
