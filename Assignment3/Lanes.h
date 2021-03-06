/**
 * Lane class. Implements a shooting gallery lane for 2 users.
 * Not thread safe and requires external synchronization to manipulate lanes.
 * If you want add more than 2 users. Add to the Colors and
 */

#ifndef LANES_H
#define LANES_H

#include <iostream>
#include <iomanip>
#include <ostream>
#include <assert.h>


/**
 * Thanks to Skurmedel for this code.
 */

enum Color {
    white = 0,
    red = 1,
    blue = 2,
    violet = 3
};


// Used in print()
namespace Colors {
    enum Code {
        FG_RED      = 31,
        FG_GREEN    = 32,
        FG_BLUE     = 34,
        FG_DEFAULT  = 39,
        BG_RED      = 41,
        BG_GREEN    = 42,
        BG_BLUE     = 44,
        BG_DEFAULT  = 49
    };
    
    class Modifier {
        Code code;
    public:
        Modifier(Code pCode) : code(pCode) {}
        friend std::ostream&
        operator<<(std::ostream& os, const Modifier& mod) {
            return os << "\033[" << mod.code << "m";
        }
    };
}



class Lanes {
public:
    // Constructors
    Lanes(int nlanes);
    Lanes(const Lanes& from);
    ~Lanes();
    
    // Set color in specific lane
    Color Set(int index, Color myLaneColor);
    
    // Obtain color in specific lane
    Color Get(int index);
    
    // Return number of lanes
    int Count();
    // Print color of lane.
    void Print();
    //
    void Clear();
		//clears the lane to white from any color
		void ClearLane(int index);
    
private:
    /* the data of the Lanes class */
    Color* lanes; // Array of size nlanes. Stores color of indexed lane
    int nlanes; // number of lanes in Lanes object
};


Lanes::Lanes(int input) : nlanes(input) {
    lanes = new Color[nlanes];
}


Lanes::~Lanes() {
    delete[] lanes;
}


// Creates copy of existing Lane
Lanes::Lanes(const Lanes& inputLane) {
    nlanes = inputLane.nlanes;
    lanes = new Color[nlanes];
    
    for (int i = 0; i < nlanes; i++) {
        lanes[i] = inputLane.lanes[i];
    }
}

// Call to set the 'index' lane to myLaneColor.
// Sets lane color to myLaneColor if white or already myLaneColor otherwise sets it to violet
// Returns white upon success, the previous color upon failure
Color Lanes::Set(int index, Color myLaneColor) {
    // You should be only trying to set red or blue.
    assert ((myLaneColor == red) || (myLaneColor == blue));
    
    // If violet you are already in trouble
    if (lanes[index] == violet) {
        return violet;
    }
    
    // If not white, set color to violet if opponent already shot lane else your own color
    if (lanes[index] != white) {
        Color oldColor = lanes[index];
        lanes[index] = (lanes[index] == myLaneColor)? myLaneColor : violet;
        
        return oldColor;
    }
    
    // If I got here then I am sure my lane is white.
    lanes[index] = myLaneColor;
    
    return white;
}


// Return color of specified lane
Color Lanes::Get(int index) {
    
    return lanes[index];
}


// Return count of lanes
int Lanes::Count() {
    return nlanes;
}


// Reset all lane colors to white
void Lanes::Clear() {
    for (int i = 0; i < nlanes; i++) {
        lanes[i] = white;
    }
}

// Reset specified lane to white
void Lanes::ClearLane(int index){
		lanes[index] = white;
}

// Print the current lane status
void Lanes::Print() {
    Colors::Modifier reds(Colors::BG_RED);
    Colors::Modifier def(Colors::BG_DEFAULT);
    Colors::Modifier blues(Colors::FG_BLUE);
    Colors::Modifier greens(Colors::BG_GREEN);
    
    for (int i = 0; i < nlanes; i++)
        std::cout << "|------";
    
    std::cout << "|";
    std::cout << std::endl;
    std::cout << "|" ;
    
    for (int i = 0; i < nlanes; i++) {
        
        switch (lanes[i]) {
            case red:
                //std::cout << reds << "##RD##";
                std::cout << " Red  ";
                break;
            case blue:
                //std::cout << blues << "##BL##";
                std::cout << " Blue ";
                break;
            case violet:
                //std::cout << greens << "##VI##";
                std::cout << "Violet";
                break;
            case white:
                //std::cout << def << "##WH##";
                std::cout << " White";
                break;
            default:
                std::cout << "Error";
        }
        //std::cout << def << "|";
        std::cout << "|";
    }
    
    std::cout << std::endl;
    for (int i = 0; i < nlanes; i++) {
        std::cout << "|------";
    }
    std::cout << "|" << std::endl << std::endl;
}

#endif
