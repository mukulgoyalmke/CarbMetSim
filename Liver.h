#ifndef __HumanBody__Liver__
#define __HumanBody__Liver__

#include <stdio.h>
#include "Blood.h"
#include "HumanBody.h"
#include "Kidneys.h"

class Liver
{
    friend class HumanBody;
    friend class SimCtl;

    double glycogen;
    double glycogenMax_;
    
    double glucoseToGlycogenInLiver_;
    double glycogenSynth_Insulin_Mean_;
    double glycogenSynth_Insulin_StdDev_;

    double glycogenToGlucoseInLiver_; // in units of mg of glucose per kg per minute
    double maxLipogenesis_; // in units of mg per minute
    
    double glycolysisMin_; // mg per kg per minute
    double glycolysisMax_;
    
    double glycolysisToLactateFraction_;

    double gngLiver_;
    
    double glucoseToNEFA_;
    
    double glucose;
    double fluidVolume_;
    double Glut2Km_;
    double Glut2VMAX_;
    
    HumanBody* body;

    double absorptionPerTick;
    double toGlycogenPerTick;
    double fromGlycogenPerTick;
    double glycolysisPerTick;
    double gngPerTick;
    double releasePerTick;
public:
    
    Liver(HumanBody* body_);
    
    void processTick();
    
    void setParams();
};

#endif /* defined(__HumanBody__Liver__) */
