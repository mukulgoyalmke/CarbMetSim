#include <iostream>
#include "Kidneys.h"
#include "Blood.h"
#include "Liver.h"

extern SimCtl* sim;

Kidneys::Kidneys(HumanBody* myBody)
{
    body = myBody;
    
    glutamineConsumed_ = 0;
    
    // 1 micromol per kg per minute = 0.1801559 mg per kg per minute
    double micromol = 0.1801559;
    gngFromLactate_ = 0.42 * 1.1 * micromol;
    gngFromGlycerol_ = 0.42 * 0.5 * micromol;
    gngFromGlutamine_ = 0.42 * 0.5 * micromol;
    gngFromAlanine_ = 0.42 * 0.1 * micromol;
    
    //Gerich: insulin dependent: 1 to 5 micromol per kg per minute
    glycolysisMin_ = 0.35 * 0.5 * 0.1801559; // mg per kg per minute
    glycolysisMax_ = 0.35 * 2.0 * 0.1801559; // mg per kg per minute
    
    reabsorptionThreshold_ = 11*180.1559/10; //mg/dl equivalent of 11 mmol/l
    glucoseExcretionRate_ = 100/(11*180.1559/10); // mg per minute per(mg/dl)
    // As BGL increases from 11 mmol/l to 22 mmol/l, glucose excretion in urine increases
    // from 0 mg/min to 100mg/min.
	totalExcretion = 0;
}

void Kidneys::processTick()
{
    double x; // to hold the random samples
    
    static std::poisson_distribution<int> glucoseExcretionRate__ (1000.0*glucoseExcretionRate_);
    static std::poisson_distribution<int> glycolysisMin__ (1000.0*glycolysisMin_);
    static std::poisson_distribution<int> gngFromLactate__ (1000.0*gngFromLactate_);
    static std::poisson_distribution<int> gngFromGlycerol__ (1000.0*gngFromGlycerol_);
    static std::poisson_distribution<int> gngFromGlutamine__ (1000.0*gngFromGlutamine_);
    static std::poisson_distribution<int> gngFromAlanine__ (1000.0*gngFromAlanine_);
    
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

    double scale = body->insulinImpactOnGNG();
    // from non-lactate sources
    double gng =  (double)(gngFromGlycerol__(sim->generator))
                + (double)(gngFromGlutamine__(sim->generator))
                + (double)(gngFromAlanine__(sim->generator));
    gng *= scale * (body->gngImpact_) * (body->bodyWeight)/1000.0;
    if( gng > 0 )
    {
    	gngPerTick = gng;
    }

    // from lactate
    gng = (double)(gngFromLactate__(sim->generator));
    gng *= scale * (body->gngImpact_) * (body->bodyWeight)/1000.0;
	//cout << "Puzzle2 " << gng << " ";
    gng = body->blood->consumeGNGSubstrates(gng);
	//cout << gng << endl;
    if( gng > 0 )
    {
    	gngPerTick += gng;
    }

    body->blood->addGlucose(gngPerTick);
    releasePerTick = gngPerTick;
/************************************
    x = (double)(gngFromLactateRate__(sim->generator));
    x *= body->bodyWeight/1000.0;
    x = body->blood->gngFromHighLactate(x);
    if( x > 0 )
    {
    	glucose += x;
    	//SimCtl::time_stamp();
    	//cout << " GNG from lactate in Kidneys " << x << "mg" << endl;
    }
    gngPerTick += x;

    //cout << "After GNG in kidney, glucose in kideny " << glucose << " mg, blood lactate " << body->blood->lactate << " mg" << endl;
    
    if( body->blood->glutamine > glutamineConsumed_ )
    {
        body->blood->glutamine -= glutamineConsumed_;
    }
    else
    {
        body->blood->glutamine = 0;
    }
********************************************/
    
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
    
/*
    totalExcretion += excretionPerTick;
    SimCtl::time_stamp();
    cout << " Kidneys:: Absorption " << absorptionPerTick << endl;
    SimCtl::time_stamp();
    cout << " Kidneys:: Release " << releasePerTick << endl;
    SimCtl::time_stamp();
    cout << " Kidneys:: Glycolysis " << glycolysisPerTick << endl;
    SimCtl::time_stamp();
    cout << " Kidneys:: GNG " << gngPerTick << endl;
    SimCtl::time_stamp();
    cout << " Kidneys:: TotalExcretion " << totalExcretion << endl;
*/
    //SimCtl::time_stamp();
    //cout << " Kidneys:: Excretion " << excretionPerTick << endl;
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
        if(itr->first.compare("glutamineConsumed_") == 0)
        {
            glutamineConsumed_ = itr->second;
        }
        
    }
}

