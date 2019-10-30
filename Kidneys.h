#ifndef __HumanBody__Kidneys__
#define __HumanBody__Kidneys__

class HumanBody;

class Kidneys
{
    friend class HumanBody;
    friend class Liver;

    double glycolysisMin_;
    double glycolysisMax_;
    
    // mg per kg per minute
    double gngKidneys_;

    double reabsorptionThreshold_;
    double glucoseExcretionRate_;

    HumanBody* body;
    
    double absorptionPerTick;
    double releasePerTick;
    double glycolysisPerTick;
    double gngPerTick;
    double excretionPerTick;
    double totalExcretion;
public:
    
    Kidneys(HumanBody* myBody);
    
    void processTick();
    void setParams();
};

#endif /* defined(__HumanBody__Kidneys__) */
