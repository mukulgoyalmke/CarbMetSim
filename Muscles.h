#ifndef __HumanBody__Muscles__
#define __HumanBody__Muscles__

class HumanBody;
class Liver;

class Muscles
{
    friend class HumanBody;
    friend class SimCtl;
    friend class Liver;
    double glycogen;
    double glycogenMax_;
    double glycogenShareDuringExerciseMean_;
    double glucose; // mg
    double volume_; //in dl
    
    double basalGlucoseAbsorbed_;
    double maxGlucoseAbsorptionDuringExercise_;
    double baaToGlutamine_;
    double glycolysisMin_;
    double glycolysisMax_;

    double glucoseToGlycogen_;

    double Glut4Km_;
    double Glut4VMAX_; // mg per kg per minute
    double PeakGlut4VMAX_; // mg per kg per minute

    HumanBody* body;

    double glucoseAbsorbedPerTick;
    double glycogenSynthesizedPerTick;
    double glycogenBreakdownPerTick;
    double oxidationPerTick;
    double glycogenOxidizedPerTick;
    double glycolysisPerTick;
    double totalGlucoseAbsorbed;
public:
    
    //Set default values
    Muscles(HumanBody* myBody);
    
    void processTick();
    void setParams();
};

#endif /* defined(__HumanBody__Muscle__) */
