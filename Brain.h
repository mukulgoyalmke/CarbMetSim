#ifndef __HumanBody__Brain__
#define __HumanBody__Brain__

#include <stdio.h>
#include "HumanBody.h"

class Brain
{
    friend class HumanBody;

    double glucoseOxidized_; // mg per minute
    double glucoseToAlanine_;
    double bAAToGlutamine_;
    double oxidationPerTick;
    HumanBody* body;
public:
    
    Brain(HumanBody* myBody);
    void processTick();
    void setParams();
};

#endif /* defined(__HumanBody__Brain__) */
