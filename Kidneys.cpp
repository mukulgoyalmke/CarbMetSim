#include <iostream>
#include "Kidneys.h"
#include "Blood.h"
#include "Liver.h"

extern SimCtl* sim;

Kidneys::Kidneys(HumanBody* myBody)
{
    body = myBody;
    
    // 1 micromol per kg per minute = 0.1801559 mg per kg per minute
    //double micromol = 0.1801559;
    //gng_ = 0.42 * 2.2 * micromol;
    gngKidneys_ = 0.16;
    
    //Gerich: insulin dependent: 1 to 5 micromol per kg per minute
    glycolysisMin_ = 0.35 * 0.5 * 0.1801559; // mg per kg per minute
    glycolysisMax_ = 0.9 * 0.35 * 2.0 * 0.1801559; // mg per kg per minute
    
    reabsorptionThreshold_ = 11*180.1559/10; //mg/dl equivalent of 11 mmol/l
    glucoseExcretionRate_ = 100/(11*180.1559/10); // mg per minute per(mg/dl)
    // As BGL increases from 11 mmol/l to 22 mmol/l, glucose excretion in urine increases
    // from 0 mg/min to 100mg/min.
	totalExcretion = 0;
}

void Kidneys::processTick()
{
    double x; // to hold the random samples
    
    static std::poisson_distribution<int> rand__ (100);
    static std::poisson_distribution<int> glucoseExcretionRate__ (1000.0*glucoseExcretionRate_);
    static std::poisson_distribution<int> glycolysisMin__ (1000.0*glycolysisMin_);
    
    absorptionPerTick = 0;
    releasePerTick = 0;

    //Glycolysis. Depends on insulin level. Some of the consumed glucose becomes lactate.
    
    //Gerich says:
    //The metabolic fate of glucose is different in different regions of the kidney. Because of its low oxygen tension, and low levels of oxidative enzymes, the renal medulla is an obligate user of glucose for its energy requirement and does so anaerobically. Consequently, lactate is the main metabolic end product of glucose taken up in the renal medulla, not carbon dioxide (CO2) and water. In contrast, the renal cortex has little  glucose phosphorylating capacity but a high level of oxidative enzymes. Consequently, this part of the kidney does not take up and use very much glucose, with oxidation of FFAs acting as the main source of energy. A major energy-requiring process in the kidney is the reabsorption of glucose from glomerular filtrate in the proximal convoluted tubule.
   
    x = (double)(glycolysisMin__(sim->generator))/1000.0;
    double toGlycolysis = body->glycolysis(x,glycolysisMax_);
    
    body->blood->removeGlucose(toGlycolysis);
    body->blood->lactate += toGlycolysis;
    absorptionPerTick = toGlycolysis;
    glycolysisPerTick = toGlycolysis;
 
   //gluconeogenesis.
    double gng = gngKidneys_;
    gng *= body->insulinImpactOnGNG();
    //gng *= (double)(rand__(sim->generator))/100.0;
    gng *= (0.9 + (double)(rand__(sim->generator))/1000.0);
    gng *= body->bodyWeight;
    gngPerTick = gng;
    body->blood->addGlucose(gng);
    releasePerTick = gng;

    //Glucose excretion in urine
    
    double bgl = body->blood->getBGL();

    excretionPerTick = 0;
    if( bgl > reabsorptionThreshold_ )
    {
        x = (double)(glucoseExcretionRate__(sim->generator));
	x = x/1000.0;
        excretionPerTick = (body->excretionKidneysImpact_)*x*(bgl-reabsorptionThreshold_);
        body->blood->removeGlucose(excretionPerTick);
    }
    
    totalExcretion += excretionPerTick;
/*
    SimCtl::time_stamp();
    cout << " Kidneys:: Absorption " << absorptionPerTick << endl;
    SimCtl::time_stamp();
    cout << " Kidneys:: Release " << releasePerTick << endl;
*/
    SimCtl::time_stamp();
    cout << " Kidneys:: Glycolysis " << glycolysisPerTick << endl;
    SimCtl::time_stamp();
    cout << " Kidneys:: TotalExcretion " << totalExcretion << endl;
    SimCtl::time_stamp();
    cout << " Kidneys:: GNG " << gngPerTick << endl;
    SimCtl::time_stamp();
    cout << " Kidneys:: Excretion " << excretionPerTick << endl;
}

void Kidneys::setParams()
{
    for( ParamSet::iterator itr = body->metabolicParameters[body->bodyState][KIDNEY].begin();
        itr != body->metabolicParameters[body->bodyState][KIDNEY].end(); itr++)
    {
        if(itr->first.compare("glycolysisMin_") == 0)
        {
            glycolysisMin_ = itr->second;
        }
        if(itr->first.compare("glycolysisMax_") == 0)
        {
            glycolysisMax_ = itr->second;
        }
        if(itr->first.compare("gngKidneys_") == 0)
        {
            gngKidneys_ = itr->second;
        }
    }
}

