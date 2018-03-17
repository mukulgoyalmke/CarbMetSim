#include <iostream>
#include "Kidneys.h"
#include "Blood.h"
#include "Liver.h"

Kidneys::Kidneys(HumanBody* myBody)
{
    body = myBody;
    
    glutamineConsumed_ = 0;
    
    glucose = 0;
    fluidVolume_ = 10.0; //dl
    
    // 2.5 micromol per kg per minute = 2.5*180.1559/1000 mg per kg per minute = 0.45038975 mg per kg per minute
    gluconeogenesisRate_ = 2.0*0.45038975;
    gngFromLactateRate_ = 9*gluconeogenesisRate_; // by default
    
    Glut2VMAX_ = 30; // mg per kg per minute
    Glut2Km_ = 20*180.1559/10.0; // mg/deciliter equal to 20 mmol/l (Frayn Table 2.2.1)
    Glut1Rate_ = 1; // mg per kg per minute
    
    //Gerich: insulin dependent: 1 to 5 micromol per kg per minute
    glycolysisMin_ = 0.1801559; // mg per kg per minute
    glycolysisMax_ = 5*glycolysisMin_;
    
    reabsorptionThreshold_ = 11*180.1559/10; //mg/dl equivalent of 11 mmol/l
    glucoseExcretionRate_ = 100/(11*180.1559/10); // mg per minute per(mg/dl)
    // As BGL increases from 11 mmol/l to 22 mmol/l, glucose excretion in urine increases from 0 mg/min to 100mg/min.
}

void Kidneys::processTick()
{
    double x; // to hold the random samples
    
    static std::poisson_distribution<int> glucoseExcretionRate__ (1000.0*glucoseExcretionRate_);
    static std::poisson_distribution<int> glycolysisMin__ (1000.0*glycolysisMin_);
    static std::poisson_distribution<int> gngRate__ (1000.0*gluconeogenesisRate_);
    static std::poisson_distribution<int> gngFromLactateRate__ (1000.0*gngFromLactateRate_);
    static std::poisson_distribution<int> Glut2VMAX__ (1000.0*Glut2VMAX_);
    static std::poisson_distribution<int> basalAbsorption__ (1000.0*Glut1Rate_);
    
    double bgl = body->blood->getBGL();
    double glInKidneys = glucose/fluidVolume_;
    
    x = (double)(Glut2VMAX__(SimCtl::myEngine()));
    x *= body->bodyWeight/1000.0;
    double y = (double)(basalAbsorption__(SimCtl::myEngine()));
    y *= body->bodyWeight/1000.0;
    
    if( glInKidneys < bgl )
    {
        double diff = bgl - glInKidneys;
        double g = (1 + body->insulinResistance_)*x*diff/(diff + Glut2Km_);
        // uptake increases for higher insulin resistance.
        // may want to change this formula later - Mukul
        g += y; // basal absorption
        
        body->blood->removeGlucose(g);
        glucose += g;
    	//SimCtl::time_stamp();
        //cout << " Kidneys removing " << g << " mg of glucose frm blood, basal " << y << endl;
	glucoseAbsorptionPerTick = g;
    }
    else
    {
        double diff = glInKidneys - bgl;
        double g = (1 + body->insulinResistance_)*x*diff/(diff + Glut2Km_);
        //g += y;
        
        if( g > glucose )
        {
            cout << "Releasing more glucose to blood than what is present in liver!\n";
            exit(-1);
        }
        
        glucose -= g;
        body->blood->addGlucose(g);
    	//SimCtl::time_stamp();
        //cout << " Kidneys releasing " << g << " mg of glucose to blood" << endl;
	glucoseAbsorptionPerTick = -1*g;
    }
    
    //Glycolysis. Depends on insulin level. Some of the consumed glucose becomes lactate.
    
    //Gerich says:
    //The metabolic fate of glucose is different in different regions of the kidney. Because of its low oxygen tension, and low levels of oxidative enzymes, the renal medulla is an obligate user of glucose for its energy requirement and does so anaerobically. Consequently, lactate is the main metabolic end product of glucose taken up in the renal medulla, not carbon dioxide (CO2) and water. In contrast, the renal cortex has little  glucose phosphorylating capacity but a high level of oxidative enzymes. Consequently, this part of the kidney does not take up and use very much glucose, with oxidation of FFAs acting as the main source of energy. A major energy-requiring process in the kidney is the reabsorption of glucose from glomerular filtrate in the proximal convoluted tubule.
    
    double scale = (1.0 - body->insulinResistance_)*(body->blood->insulinLevel);
    
    x = (double)(glycolysisMin__(SimCtl::myEngine()));
    x *= body->bodyWeight/1000.0;
    if( x > glycolysisMax_*(body->bodyWeight))
        x = glycolysisMax_*(body->bodyWeight);
    
    double toGlycolysis = x + scale* ( (glycolysisMax_*(body->bodyWeight)) - x);
    
    if( toGlycolysis > glucose)
        toGlycolysis = glucose;
    glucose -= toGlycolysis;
    body->blood->lactate += toGlycolysis;
    //SimCtl::time_stamp();
    //cout << " Glycolysis in kidney " << toGlycolysis << " , blood lactate " << body->blood->lactate << " mg" << endl;
   glycolysisPerTick = toGlycolysis;
 
   //gluconeogenesis. Depends on insulin level and on substrate concentration.
    
    //4. release some glucose by consuming lactate/alanine/glycerol (gluconeogenesis)(the amount depends on body state and the concentration of lactate/alanine/glycerol in blood; when insulin is high (fed state) glycolysis is favored and when glucagon high (compared to insulin; starved state) gluconeogenesis is favored)
    
    scale = 1 - (body->blood->insulinLevel)*(1 - (body->insulinResistance_));
    x = (double)(gngRate__(SimCtl::myEngine()));
    x *= body->bodyWeight/1000.0;
    double gng = x *scale;
    gng = body->blood->consumeGNGSubstrates(gng);
    if( gng > 0 )
    {
    	glucose += gng;
    	//SimCtl::time_stamp();
    	//cout << " GNG in Kidneys " << gng << "mg" << endl;
    }
    gngPerTick = gng;
    
    x = (double)(gngFromLactateRate__(SimCtl::myEngine()));
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
    
    //Glucose excretion in urine
    
    bgl = body->blood->getBGL();

    excretionPerTick = 0;
    if( bgl > reabsorptionThreshold_ )
    {
        x = (double)(glucoseExcretionRate__(SimCtl::myEngine()));
	x = x/1000.0;
        excretionPerTick = x*(bgl-reabsorptionThreshold_);
        body->blood->removeGlucose(excretionPerTick);
        
    	//SimCtl::time_stamp();
        //cout << " glucose excretion in urine " << g << endl;
    }
    
    SimCtl::time_stamp();
    cout << " Kidneys:: GlucoseAbsorption " << glucoseAbsorptionPerTick << endl;
    SimCtl::time_stamp();
    cout << " Kidneys:: Glycolysis " << glycolysisPerTick << endl;
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
        if(itr->first.compare("fluidVolume_") == 0)
        {
            fluidVolume_ = itr->second;
        }
        if(itr->first.compare("Glut2VMAX_") == 0)
        {
            Glut2VMAX_ = itr->second;
        }
        if(itr->first.compare("Glut2Km_") == 0)
        {
            Glut2Km_ = itr->second;
        }
        if(itr->first.compare("Glut1Rate_") == 0)
        {
            Glut1Rate_ = itr->second;
        }
        if(itr->first.compare("glycolysisMin_") == 0)
        {
            glycolysisMin_ = itr->second;
        }
        if(itr->first.compare("glycolysisMax_") == 0)
        {
            glycolysisMax_ = itr->second;
        }
        if(itr->first.compare("gluconeogenesisRate_") == 0)
        {
            gluconeogenesisRate_ = itr->second;
        }
        if(itr->first.compare("glutamineConsumed_") == 0)
        {
            glutamineConsumed_ = itr->second;
        }
        
    }
}

