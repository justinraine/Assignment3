
all: Shooter

Shooter: Shooter.o 
	g++ -mrtm -std=c++11 -pthread $< -o $@

Shooter.o: Shooter.cpp
	g++ -c -mrtm -std=c++11 -pthread $<

clean:
	rm -rf *o Shooter