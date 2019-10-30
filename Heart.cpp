#include "Heart.h"
#include "Blood.h"
#include <stdlib.h>
#include <iostream>

extern SimCtl* sim;

Heart::Heart(HumanBody* mybody)
{
    body = mybody;

    lactateOxidized_ = 0;
    
    basalGlucoseAbsorbed_ = 14; //mg per minute
    //Skeletal Muscle Glycolysis, Oxidation, and Storage of an Oral Glucose Load- Kelley et.al.
    
    Glut4Km_ = 5*180.1559/10.0; //mg/dl equivalent of 5 mmol/l
    Glut4VMAX_ = 0; //mg per kg per minute
}


void Heart::processTick()
{
    //double basalAbsorption = basalGlucoseAbsorbed_*(body->bodyWeight);
    static std::poisson_distribution<int> basalGlucoseAbsorbed__(1000.0*basalGlucoseAbsorbed_);
    //static std::poisson_distribution<int> lactateOxidized__(1000.0*lactateOxidized_);
    
    double basalAbsorption = (double)(basalGlucoseAbsorbed__(sim->generator))/1000.0;
    
    body->blood->removeGlucose(basalAbsorption);
    
    oxidationPerTick = basalAbsorption;

   /********** 
    // Absorption via GLUT4
    double bgl = body->blood->getBGL();
    double scale = (1.0 - body->insulinResistance_)*(body->blood->insulinLevel)*(body->bodyWeight);
    double g = scale*Glut4VMAX_*bgl/(bgl + Glut4Km_);
    
    body->blood->removeGlucose(g);

    oxidationPerTick += g;
   ***************/

    SimCtl::time_stamp();
    cout << " Heart:: Oxidation " << oxidationPerTick << endl;
/*
    double lactateOxidized = (double)(lactateOxidized__(sim->generator))/1000.0;
    if( body->blood->lactate >= lactateOxidized )
    {
        body->blood->lactate -= lactateOxidized;
    }
    else
    {
        body->blood->lactate = 0;
    }
*/
}

void Heart::setParams()
{
    for( ParamSet::iterator itr = body->metabolicParameters[body->bodyState][HEART].begin();
        itr != body->metabolicParameters[body->bodyState][HEART].end(); itr++)
    {

        if(itr->first.compare("lactateOxidized_") == 0)
        {
            lactateOxidized_ = itr->second;
        }
        
        if(itr->first.compare("basalGlucoseAbsorbed_") == 0)
        {
            basalGlucoseAbsorbed_ = itr->second;
        }
        
        if(itr->first.compare("Glut4Km_") == 0)
        {
            Glut4Km_ = itr->second;
        }
        
        if(itr->first.compare("Glut4VMAX_") == 0)
        {
            Glut4VMAX_ = itr->second;
        }
    }
}
