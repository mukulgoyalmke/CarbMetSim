#ifndef __HumanBody__Kidneys__
#define __HumanBody__Kidneys__

class HumanBody;

class Kidneys
{
    friend class HumanBody;

    double glucose;
    double fluidVolume_;
    double Glut2VMAX_; // mg per kg per minute
    double Glut2Km_;
    double Glut1Rate_;
    
    double glycolysisMin_;
    double glycolysisMax_;
    
    double gluconeogenesisRate_; // mg per kg per minute
    double gngFromLactateRate_;
    
    double glutamineConsumed_;
    
    double reabsorptionThreshold_;
    double glucoseExcretionRate_;

    HumanBody* body;
    
    double glucoseAbsorptionPerTick;
    double glycolysisPerTick;
    double gngPerTick;
    double excretionPerTick;

public:
    
    Kidneys(HumanBody* myBody);
    
    void processTick();
    void setParams();
};

#endif /* defined(__HumanBody__Kidneys__) */
