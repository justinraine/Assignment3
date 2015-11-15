#include <iostream>
#include <thread>
#include <unistd.h>
#include "Lanes.h"
#include <vector>
#include <mutex>
#include <chrono>
#include <getopt.h>
#include "tsx-tools/rtm.h"
#include <condition_variable>

using namespace std;


// Debug setting
#define DEBUG_OUTPUT false
#define GRAPHIC_OUTPUT false

#define DB(fmt, ...) \
do { if (DEBUG_OUTPUT) fprintf(stderr, fmt, __VA_ARGS__); } while (0)



// Specify action with definition of: {RogueCoarse, RogueFine, RogueTM, RogueCoarse2, RogueFine2, RogueTM2,
//                                     RogueCoarseCleaner, RogueFineCleaner, RogueTMCleaner}
#define RogueTM

#define RogueCoarseCleaner

/*
 * You can also choose RogueCoarse type by calling:
 *		make ROGUE=-DRogueCoarse
 * Replace RogueCoarse with the Rogue you want to run
 * Must delete existing Shooter and Shooter.o from build folder to change Rogue type
 */


// Global variables
Lanes* lanesGallery;
int nLanes = 10;
int roundLanesShot = 0; //number of lanes shot (blue or red)
int redRoundLanesShot = 0; //for finegrain cases
int blueRoundLanesShot = 0; //for finegrain cases
int roundsTotal; //total number of rounds to run
int roundsCount; //rounds executed so far
int successfulTransactions = 0;
int abortedTransactions = 0;
int TMFallbackSuccesses = 0;
int synchronizationErrors = 0; // ie. non-white lane shot
int shotAttempts = 0;
int nonWhiteSelections = 0; // Abort if selected lane is not white
bool roundSummaryPrinted = false;
chrono::time_point<chrono::system_clock> roundStartTime;
mutex coarseLock;
mutex * fineLock = new mutex [nLanes];
mutex fineCountLock[2]; //lock to protect counts of lanes shot by each thread. 0 for red, 1 for blue
condition_variable cv; //used to wake up and wait for cleaner
mutex cleanerLock; //used to wake up and wait for cleaner
bool lanesFull = false; // for cleaner conditional variable
bool lanesCleaned = false; //for cleaner conditional variable
mutex printLock;


// Function prototypes
bool unsafeSetupNextRound();
void calculateShotRates(double &redShotRate, double &blueShotRate);
void unsafePrintRoundSummary(double redShotRate, double blueShotRate);
void safePrintRoundSummary(double redShotRate, double blueShotRate);
void unsafeUpdateRoundGlobalVariables();

Color transactionSafeGetLaneColor(int selectedLane);
//void transactionSafeSetLaneColor(int selectedLane, Color playerColor);
bool tryTransactionSafeShot(int selectedLane, Color playerColor);
bool tryTransactionSafeShot2(int selectedLane, int selectedLane2, Color playerColor);
void transactionSafeSetupNextRound();





void shooterAction(int rateShotsPerSecond, Color playerColor) {
    roundStartTime = chrono::system_clock::now();
    srand ((int)chrono::system_clock::now().time_since_epoch().count() / playerColor); // seed random number
    
    // Setup variables
    __attribute__((unused))Color selectedLaneColor;
    __attribute__((unused))Color selectedLaneColor2; // Surpress unsused variable warnings in single lane cases. Only compatable with gcc
    __attribute__((unused))Color returnColor;
    __attribute__((unused))Color returnColor2;
    //int violetLanes = 0;  *** Should never have violet lanes. Use assert statements instead
    
    
    while (roundsCount < roundsTotal) {
        auto timeOfNextShot = chrono::system_clock::now() + chrono::milliseconds(1000/rateShotsPerSecond);

        // Pick random lane
        int selectedLane = rand() % nLanes;
        int selectedLane2;
        do {
            selectedLane2 = rand() % nLanes;
        } while (selectedLane == selectedLane2);
        
#pragma mark RogueCoarse
#ifdef RogueCoarse
        // Check color of selected lane
        coarseLock.lock();
        
        // If the other thread cleaned up the final round, then stop looping
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
        coarseLock.unlock();
        
        assert(returnColor == white); // If synchronizing correctly, will always be white
        roundLanesShot++; //the lane was shot successfully, so one more lane has been shot this round
        if (playerColor == red) {
            redRoundLanesShot++;
        } else {
            blueRoundLanesShot++;
        }
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
        
        
        
#pragma mark RogueFine
#ifdef RogueFine
        // Check color of selected lane
        fineLock[selectedLane].lock();
        
        //if the other thread cleaned up the final round, then stop looping
        if (roundsCount >= roundsTotal){
            fineLock[selectedLane].unlock();
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
        
        
        
#pragma mark RogueTM
#ifdef RogueTM
        //if the other thread cleaned up the final round, then stop looping
        if (roundsCount >= roundsTotal){
            break;
        }
        
        DB("Player %u selected lane %d, currently %u\n", playerColor, selectedLane, transactionSafeGetLaneColor(selectedLane));
        
        if (tryTransactionSafeShot(selectedLane, playerColor) == false) {
            this_thread::sleep_until(timeOfNextShot);
            continue;
        } // else selectedLane was sucessfully shot playerColor
        
        DB("Player %u shot lane %d\n", playerColor, selectedLane);
        
        // lanes are full when shotCount == total number of lanes
        if (roundLanesShot >= nLanes || (redRoundLanesShot + blueRoundLanesShot >= nLanes)) {
            double redShotRate, blueShotRate;
            calculateShotRates(redShotRate, blueShotRate);
            safePrintRoundSummary(redShotRate, blueShotRate);
            transactionSafeSetupNextRound();
        }
        
        // Sleep to control shots to rateShotsPerSecond
        this_thread::sleep_until(timeOfNextShot);
#endif
        
        
        
#pragma mark RogueCoarse2
#ifdef RogueCoarse2
        // Check color of selected lane
        coarseLock.lock();
        
        // If the other thread cleaned up the final round, then exit while loop
        if (roundsCount >= roundsTotal){
            coarseLock.unlock();
            break;
        }
        
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
        
        
        
#pragma mark RogueFine2
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
        
        
        
#pragma mark RogueTM2
#ifdef RogueTM2
        //if the other thread cleaned up the final round, then stop looping
        if (roundsCount >= roundsTotal){
            break;
        }
        
        DB("Player %u selected lanes %d (%u) and %d(%u)\n",
           playerColor, selectedLane, selectedLaneColor, selectedLane2, selectedLaneColor2);
        
        if (tryTransactionSafeShot2(selectedLane, selectedLane2, playerColor) == false) {
            this_thread::sleep_until(timeOfNextShot);
            continue;
        } // else selectedLane was sucessfully shot playerColor
        
        DB("Player %u shot lanes %d and %d\n", playerColor, selectedLane, selectedLane2);
        
        // lanes are full when shotCount == total number of lanes
        if (roundLanesShot >= nLanes || (redRoundLanesShot + blueRoundLanesShot >= nLanes)) {
            double redShotRate, blueShotRate;
            calculateShotRates(redShotRate, blueShotRate);
            safePrintRoundSummary(redShotRate, blueShotRate);
            transactionSafeSetupNextRound();
        }
        
        // Sleep to control shots to rateShotsPerSecond
        this_thread::sleep_until(timeOfNextShot);
#endif
        
        
        
#pragma mark RogueCoarseCleaner
#ifdef RogueCoarseCleaner
        // Check color of selected lane
        coarseLock.lock();
        
        // If the cleaner thread cleaned up the final round, then stop looping
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
        coarseLock.unlock();
        
        assert(returnColor == white); // If synchronizing correctly, will always be white
        roundLanesShot++; //the lane was shot successfully, so one more lane has been shot this round
        if (playerColor == red) {
            redRoundLanesShot++;
        } else {
            blueRoundLanesShot++;
        }

 				DB("Player %u shot lane %d\n", playerColor, selectedLane);
				
				//wake up cleaner if lanes full
				if (redRoundLanesShot + blueRoundLanesShot == nLanes){
					{
		      	std::lock_guard<std::mutex> uLock(cleanerLock);
						lanesFull = true;
		  		}
		  		cv.notify_one();
	 
					// wait for the cleaner
					{
						  std::unique_lock<std::mutex> uLock(cleanerLock);
							
						  cv.wait(uLock,[]{return lanesCleaned;});
					}
				}
        
        // Sleep to control shots to rateShotsPerSecond
        this_thread::sleep_until(timeOfNextShot);
#endif
        
        
        
#pragma mark RogueFineCleaner
#ifdef RogueFineCleaner
				// Check color of selected lane
        fineLock[selectedLane].lock();
        
        //if the cleaner thread cleaned up the final round, then stop looping
        if (roundsCount >= roundsTotal){
            fineLock[selectedLane].unlock();
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
        
				//wake up cleaner if lanes full
				if (redRoundLanesShot + blueRoundLanesShot >= nLanes){
					{
		      	std::lock_guard<std::mutex> uLock(cleanerLock);
						lanesFull = true;
		  		}
		  		cv.notify_one();
	 
					// wait for the cleaner
					{
						  std::unique_lock<std::mutex> uLock(cleanerLock);
						  cv.wait(uLock, []{return lanesCleaned;});
					}
				}

        // Sleep to control shots to rateShotsPerSecond

        this_thread::sleep_until(timeOfNextShot);
#endif
        
        
        
#pragma mark RogueTMCleaner
#ifdef RogueTMCleaner
				//if the cleaner thread cleaned up the final round, then stop looping
        if (roundsCount >= roundsTotal){
            break;
        }
        
        DB("Player %u selected lane %d, currently %u\n", playerColor, selectedLane, transactionSafeGetLaneColor(selectedLane));
        
        if (tryTransactionSafeShot(selectedLane, playerColor) == false) {
            this_thread::sleep_until(timeOfNextShot);
            continue;
        } // else selectedLane was sucessfully shot playerColor
        
        DB("Player %u shot lane %d\n", playerColor, selectedLane);
        
				//wake up cleaner if lanes full
				if (redRoundLanesShot + blueRoundLanesShot >= nLanes){
					{
		      	std::lock_guard<std::mutex> uLock(cleanerLock);
						lanesFull = true;
		  		}
		  		cv.notify_one();
	 
					// wait for the cleaner
					{
						  std::unique_lock<std::mutex> uLock(cleanerLock);
						  cv.wait(uLock, []{return lanesCleaned;});
					}
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
		while (roundsCount < roundsTotal) {

   	// Wait until woken up by a shooter
    std::unique_lock<std::mutex> uLock(cleanerLock);

		// check to make sure wake-up was not spurious
    	cv.wait(uLock, []{return lanesFull;});
		
#ifdef RogueCoarseCleaner
		 // lanes are full when shotCount == total number of lanes
        if (roundLanesShot == nLanes) {
            coarseLock.lock();
            unsafeSetupNextRound();
						lanesFull = false;
            coarseLock.unlock();
        }
#endif


#ifdef RogueFineCleaner
     		// lanes are full when shotCount + violetLanes == total number of lanes
        if ((redRoundLanesShot + blueRoundLanesShot) >= nLanes) {
            for (int i=0; i < nLanes; i++){
                fineLock[i].lock();
            }
            fineCountLock[0].lock();
            fineCountLock[1].lock();
            
            unsafeSetupNextRound();
						lanesFull = false;
            
            for (int i=0; i < nLanes; i++){
                fineLock[i].unlock();
            }
						fineCountLock[0].unlock();
            fineCountLock[1].unlock();
        }
#endif
    

#ifdef RogueTMCleaner
        // lanes are full when shotCount == total number of lanes
        if (roundLanesShot >= nLanes || (redRoundLanesShot + blueRoundLanesShot >= nLanes)) {
            double redShotRate, blueShotRate;
            calculateShotRates(redShotRate, blueShotRate);
            safePrintRoundSummary(redShotRate, blueShotRate);
            transactionSafeSetupNextRound();
						lanesFull = false;
        }
#endif

    		// Wake up shooter
				lanesCleaned = true;
			  uLock.unlock();
    		cv.notify_all();
		}
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


#pragma mark Helper Functions

// ************************
// *** Helper Functions ***
// ************************

// If lanesGallery is full, this function will output the round info, clear the lanes back to white, and start the next round
// WARNING: Not threadsafe. Provide synchronization in calling function.
bool unsafeSetupNextRound() {
    //make sure other thread did not already clean
    if (roundLanesShot >= nLanes || (redRoundLanesShot + blueRoundLanesShot) >= nLanes) {
        double redShotRate, blueShotRate;
        calculateShotRates(redShotRate, blueShotRate);
        unsafePrintRoundSummary(redShotRate, blueShotRate);
        lanesGallery->Clear();
        unsafeUpdateRoundGlobalVariables();
        return true;
    } else {
        return false;
    }
}

void calculateShotRates(double &redShotRate, double &blueShotRate) {
    // Get end time of round
    auto roundEndTime = chrono::system_clock::now();
    
    // Calculate red and blue shot rates
    chrono::duration<double> roundDuration = roundEndTime-roundStartTime;
    redShotRate = redRoundLanesShot/(double)roundDuration.count();
    blueShotRate = blueRoundLanesShot/(double)roundDuration.count();
}

void unsafePrintRoundSummary(double redShotRate, double blueShotRate) {
    // Print lanesGallery and red and blue shot rates
    cout << endl << "Round " << roundsCount+1 <<" Summary" << endl;
    lanesGallery->Print();
    printf("Red Shot Rate: %.2f shots/second\n", redShotRate);
    printf("Blue Shot Rate: %.2f shots/second\n", blueShotRate);
#if defined(RogueTM) || defined(RogueTM2) || defined(RogueTMCleaner)
    cout << "Shot Attempts: " << shotAttempts << endl;
    cout << "Successful Transaction Shots: " << successfulTransactions << endl;
    cout << "Successful Fallback Shots: " << TMFallbackSuccesses << endl;
    printf("Abort Rate: %.2f%%\n", abortedTransactions/(double)shotAttempts*100);
    cout << "Non-white Lane Selections: " << nonWhiteSelections << endl;
    cout << "Synchronization Errors: " << synchronizationErrors << endl;
#endif
}


void safePrintRoundSummary(double redShotRate, double blueShotRate) {
    printLock.lock();
    if (!roundSummaryPrinted) {
        unsafePrintRoundSummary(redShotRate, blueShotRate);
        roundSummaryPrinted = true;
    }
    printLock.unlock();
}



// Configure global variables for next round
void unsafeUpdateRoundGlobalVariables() {
    roundStartTime = chrono::system_clock::now();
    roundLanesShot = 0;
    redRoundLanesShot = 0;
    blueRoundLanesShot = 0;
    roundsCount++;
    roundSummaryPrinted = false;
    abortedTransactions = 0;
    successfulTransactions = 0;
    TMFallbackSuccesses = 0;
    synchronizationErrors = 0;
    shotAttempts = 0;
    nonWhiteSelections = 0;
}


#pragma mark Transaction Functions
#if defined(RogueTM) || defined(RogueTM2) || defined(RogueTMCleaner)

// *****************************
// *** Transaction Functions ***
// *****************************


Color transactionSafeGetLaneColor(int selectedLane) {
    mutex fallbackLock;
    bool inFallbackPath;
    Color selectedLaneColor;
    
    if(_xbegin() == -1) {
        if(inFallbackPath) {
            _xabort();
        }
        
        selectedLaneColor = lanesGallery->Get(selectedLane);
        _xend();
    }
    else{
        fallbackLock.lock();
        inFallbackPath = true;
        selectedLaneColor = lanesGallery->Get(selectedLane);
        inFallbackPath = false;
        fallbackLock.unlock();
    }
    
    return selectedLaneColor;
}


//void transactionSafeSetLaneColor(int selectedLane, Color playerColor) {
//    mutex fallbackLock;
//    bool inFallbackPath;
//    Color returnColor;
//
//    if(_xbegin() == -1) {
//        if(inFallbackPath) {
//            _xabort();
//        }
//
//        returnColor = lanesGallery->Set(selectedLane, playerColor);
//        _xend();
//    }
//    else{
//        fallbackLock.lock();
//        inFallbackPath = true;
//        returnColor = lanesGallery->Set(selectedLane, playerColor);
//        inFallbackPath = false;
//        fallbackLock.unlock();
//    }
//
//    assert(returnColor == white); // Should always return white if synchronizing correctly
//}


bool tryTransactionSafeShot(int selectedLane, Color playerColor) {
    mutex fallbackLock;
    bool inFallbackPath;
    Color selectedLaneColor;
    Color returnColor;
    shotAttempts++;
    
    if(_xbegin() == _XBEGIN_STARTED) {
        if(!inFallbackPath) {
            selectedLaneColor = lanesGallery->Get(selectedLane);
            if (selectedLaneColor != white) {
                nonWhiteSelections++;
                return false;
            }
            
            returnColor = lanesGallery->Set(selectedLane, playerColor);
            if (returnColor != white) {
                synchronizationErrors++;
                return false;
            }
            successfulTransactions++;
            roundLanesShot++; //the lane was shot successfully, so one more lane has been shot this round
            if (playerColor == red) {
                redRoundLanesShot++;
            } else {
                blueRoundLanesShot++;
            }

        } else {
            _xabort();
        }
        
        _xend();
    }
    else{
        inFallbackPath = true;
        fallbackLock.lock();
        abortedTransactions++;
        
        selectedLaneColor = lanesGallery->Get(selectedLane);
        if (selectedLaneColor != white) {
            nonWhiteSelections++;
            inFallbackPath = false;
            fallbackLock.unlock();
            return false;
        }
        
        returnColor = lanesGallery->Set(selectedLane, playerColor);
        if (returnColor != white) {
            synchronizationErrors++;
            inFallbackPath = false;
            fallbackLock.unlock();
            return false;
        }
        TMFallbackSuccesses++;
        roundLanesShot++; //the lane was shot successfully, so one more lane has been shot this round
        if (playerColor == red) {
            redRoundLanesShot++;
        } else {
            blueRoundLanesShot++;
        }
        
        inFallbackPath = false;
        fallbackLock.unlock();
    }
    
    return true;
}


bool tryTransactionSafeShot2(int selectedLane, int selectedLane2, Color playerColor) {
    mutex fallbackLock;
    bool inFallbackPath;
    Color selectedLaneColor, selectedLaneColor2, returnColor, returnColor2;
    shotAttempts += 2;
    
    if(_xbegin() == _XBEGIN_STARTED) {
        if(!inFallbackPath) {
            selectedLaneColor = lanesGallery->Get(selectedLane);
            selectedLaneColor2 = lanesGallery->Get(selectedLane2);
            if (selectedLaneColor != white || selectedLaneColor2 != white) {
                nonWhiteSelections += 2;
                return false;
            }
            
            returnColor = lanesGallery->Set(selectedLane, playerColor);
            returnColor2 = lanesGallery->Set(selectedLane2, playerColor);
            
            if (returnColor != white) {
                synchronizationErrors++;
                return false;
            }
            if (returnColor2 != white) {
                synchronizationErrors++;
                return false;
            }
            
            successfulTransactions += 2;
            roundLanesShot += 2; //the lane was shot successfully, so one more lane has been shot this round
            if (playerColor == red) {
                redRoundLanesShot += 2;
            } else {
                blueRoundLanesShot += 2;
            }
        } else {
            _xabort();
        }
        
        _xend();
    }
    else{
        inFallbackPath = true;
        fallbackLock.lock();
        abortedTransactions ++;
        
        selectedLaneColor = lanesGallery->Get(selectedLane);
        selectedLaneColor2 = lanesGallery->Get(selectedLane2);
        if (selectedLaneColor != white || selectedLaneColor2 != white) {
            nonWhiteSelections += 2;
            inFallbackPath = false;
            fallbackLock.unlock();
            return false;
        }
        
        returnColor = lanesGallery->Set(selectedLane, playerColor);
        returnColor2 = lanesGallery->Set(selectedLane2, playerColor);
        if (returnColor != white) {
            synchronizationErrors++;
            inFallbackPath = false;
            fallbackLock.unlock();
            return false;
        }
        if (returnColor2 != white) {
            synchronizationErrors++;
            inFallbackPath = false;
            fallbackLock.unlock();
            return false;
        }
        
        TMFallbackSuccesses += 2;
        roundLanesShot += 2; //the lane was shot successfully, so one more lane has been shot this round
        
        if (playerColor == red) {
            redRoundLanesShot += 2;
        } else {
            blueRoundLanesShot += 2;
        }
        
        inFallbackPath = false;
        fallbackLock.unlock();
    }
    
    return true;
}


// This function provides a transaction safe setup of the next round.
// Input parameters passed by reference and updated with calculated shot rates
void transactionSafeSetupNextRound() {
    mutex fallbackLock;
    bool inFallbackPath;
    
    if(_xbegin() == _XBEGIN_STARTED) {
        if(inFallbackPath) {
            _xabort();
        }
        
        //make sure other thread did not already clean
        if (roundLanesShot >= nLanes || (redRoundLanesShot + blueRoundLanesShot) >= nLanes) {
            lanesGallery->Clear();
            unsafeUpdateRoundGlobalVariables();
        }
        
        _xend();
    }
    else{
        fallbackLock.lock();
        inFallbackPath = true;
        
        //make sure other thread did not already clean
        if (roundLanesShot >= nLanes || (redRoundLanesShot + blueRoundLanesShot) >= nLanes) {
            lanesGallery->Clear();
            unsafeUpdateRoundGlobalVariables();
        }
        
        inFallbackPath = false;
        fallbackLock.unlock();
    }
}
#endif
