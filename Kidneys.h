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
    double gngFromLactate_;
    double gngFromGlycerol_;
    double gngFromGlutamine_;
    double gngFromAlanine_;

    double glutamineConsumed_;
    
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
