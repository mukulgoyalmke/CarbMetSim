#ifndef __HumanBody__Stomach__
#define __HumanBody__Stomach__

#include "HumanBody.h"

class Stomach
{
    double geConstant_; // mg
    double geSlopeMin_; // Min value for GE slope (applicable to fat) 
   	// the amount of gastric emptying (mg per minute) = geConstant_ + geSlope*(total food in stomach in milligrams)
 
    double RAG; //  mg
    double SAG; // mg
    double protein; //mg
    double fat; //mg

    bool stomachEmpty;
    
    HumanBody* body;
public:
    //Set Default Values
    Stomach(HumanBody* body_);
    
    void addFood(unsigned foodID, double howmuch);
    void setParams();
    void processTick();
};

#endif 
