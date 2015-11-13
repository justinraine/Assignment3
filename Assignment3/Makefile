
all: Shooter

Shooter: Shooter.o 
	g++ -mrtm -std=c++11 -pthread $(ROGUE) $< -o $@

Shooter.o: Shooter.cpp
	g++ -c -mrtm -std=c++11 -pthread $(ROGUE) $<

clean:
	rm -rf *o Shooter
