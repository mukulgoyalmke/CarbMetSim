#ifndef __HumanBody__PortalVein__
#define __HumanBody__PortalVein__

#include "HumanBody.h"

class PortalVein
{
    friend class Liver;
    double glucose; //mg
    double branchedAA; //mg
    double unbranchedAA; //mg
    double fluidVolume_;
    HumanBody* body;
public:
    PortalVein(HumanBody* body_);
    void processTick();
    void setParams();
    double getConcentration();
    void addGlucose(double g){glucose += g;}
    double getGlucose(){return glucose;}
    void removeGlucose(double g);
    void releaseAllGlucose();
    void addAminoAcids(double aa);
    void releaseAminoAcids();
};

#endif 
