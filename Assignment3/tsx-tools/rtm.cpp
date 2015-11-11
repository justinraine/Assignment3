#include <immintrin.h>
#include <iostream>
#include <vector>
#include <thread>
using namespace std;

int counter;

void ShooterAction() {

 int nretries=0;
 int status;

 if ((status = _xbegin ()) == _XBEGIN_STARTED) {

    for (int i = 0; i < 100; ++i)
    {
                counter++;
        /* code */
    }

   
        _xend ();
 }
 else {
        nretries++;
    }

    std::cout<<nretries;
}


int main()
{

    counter = 0;

    vector<thread> ths;

    ths.push_back(std::thread(&ShooterAction));
    ths.push_back(std::thread(&ShooterAction));


    for (auto& th : ths) {

        th.join();


        std::cout << counter ; 

    }




    return 0;
}
