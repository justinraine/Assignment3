#ifndef ROGUE_H

#define ROGUE_H


class Rogue {
public:
	Rogue(int color, int rate);
	~Rogue();

	/* data */
	Color Bullet; // The bullet color to paint the lane
	int ShotRate; // Rate/s required to shoot the lanes
	int Success; // Rate/s of lanes shot by ROGUE   
};


#endif