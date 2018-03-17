#include "AdiposeTissue.h"
#include "Blood.h"
#include <stdlib.h>
#include <iostream>

void AdiposeTissue::processTick()
{
    //SimCtl::time_stamp();
    //cout << " BodyWeight: " << body->bodyWeight_ << endl;
}

void AdiposeTissue::setParams()
{
}


AdiposeTissue::AdiposeTissue(HumanBody* myBody)
{
    body = myBody;
    fat = (body->fatFraction_)*(body->bodyWeight)*1000.0;
}

void AdiposeTissue::lipogenesis(double glucoseInMG)
{
    // one gram of glucose has 4kcal of energy
    // one gram of TAG has 9 kcal of energy
    //cout << "BodyWeight: Lipogenesis " << body->bodyWeight_ << " glucose " << glucoseInMG << " fat " << fat << endl;
    body->bodyWeight -= fat/1000.0;
    fat += (glucoseInMG/1000.0)*4.0/9.0;
    body->bodyWeight += fat/1000.0;
    //cout << "BodyWeight: Lipogenesis " << body->bodyWeight_ << " glucose " << glucoseInMG << " fat " << fat << endl;
}

void AdiposeTissue::consumeFat(double kcal)
{
    body->bodyWeight -= fat/1000.0;
    fat -= kcal/9.0;
    body->bodyWeight += fat/1000.0;
}

void AdiposeTissue::addFat(double newFatInMG)
{
    body->bodyWeight -= fat/1000.0;
    fat += newFatInMG/1000.0;
    body->bodyWeight += fat/1000.0;
    //cout << "BodyWeight: addFat " << body->bodyWeight_ << " newfat " << newFatInMG << endl;
}

