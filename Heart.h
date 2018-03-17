#ifndef __HumanBody__Heart__
#define __HumanBody__Heart__

#include "HumanBody.h"

class Heart
{
    friend class HumanBody;

    double basalGlucoseAbsorbed_;
    double Glut4Km_;
    double Glut4VMAX_;
    
    double oxidationPerTick;

    double lactateOxidized_;
    HumanBody* body;
    
public:
    Heart(HumanBody* mybody);
    
//The heart absorbs glucose and some lactate (depends on its concentration). Glucose is absorbed via glut4. So, high insulin concentration (i.e. fed state) is required for the heart to use glucose for its energy needs. The following paper has numbers for how much glucose and lactate is used by the heart.
    void processTick();
    void setParams();
};

#endif /* defined(__HumanBody__Heart__) */
