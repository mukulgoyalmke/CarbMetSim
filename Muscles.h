#ifndef __HumanBody__Muscles__
#define __HumanBody__Muscles__

class HumanBody;

class Muscles
{
    friend class HumanBody;
    double glycogen;
    double glycogenMax_;
    double glucose; // mg
    double volume_; //in dl
    
    double basalGlucoseAbsorbed_;
    double glucoseOxidationFraction_;
    double baaToGlutamine_;
    double glycolysisMin_;
    double glycolysisMax_;

    double Glut4Km_;
    double Glut4VMAX_; // mg per kg per minute

    HumanBody* body;

    double glucoseAbsorbedPerTick;
    double glycogenSynthesizedPerTick;
    double glycogenBreakdownPerTick;
    double oxidationPerTick;
    double glycogenOxidizedPerTick;
    double glycolysisPerTick;
public:
    
    //Set default values
    Muscles(HumanBody* myBody);
    
    void processTick();
    void setParams();
};

#endif /* defined(__HumanBody__Muscle__) */
