//
//  AdiposeTissue.h
//  HumanBody
//

#ifndef __HumanBody__AdiposeTissue__
#define __HumanBody__AdiposeTissue__

class HumanBody;

class AdiposeTissue
{
    double fat; // fat in body in grams
    HumanBody* body;
public:
    
    //Set Default Values
    AdiposeTissue(HumanBody* myBody);
    
    void processTick();
    void setParams();
    
    void lipogenesis(double glucoseInMG);
    void consumeFat(double kcal);
    void addFat(double newFatInMG);
};



#endif /* defined(__HumanBody__AdiposeTissue__) */
