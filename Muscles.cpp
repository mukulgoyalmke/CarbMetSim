#include <iostream>
#include <math.h>
#include "Muscles.h"
#include "Blood.h"
#include "AdiposeTissue.h"

extern SimCtl* sim;

void Muscles::setParams()
{
    for( ParamSet::iterator itr = body->metabolicParameters[body->bodyState][MUSCLES].begin();
        itr != body->metabolicParameters[body->bodyState][MUSCLES].end(); itr++)
    {
        if(itr->first.compare("Glycogen") == 0)
        {
            glycogen = 1000.0 * (itr->second);
        }
        if(itr->first.compare("MaxGlycogen") == 0)
        {
            glycogenMax_ = 1000.0 * (itr->second);
        }
        if(itr->first.compare("Glut4Km_") == 0)
        {
            Glut4Km_ = itr->second;
        }
        if(itr->first.compare("Glut4VMAX_") == 0)
        {
            Glut4VMAX_ = itr->second;
        }
        if(itr->first.compare("PeakGlut4VMAX_") == 0)
        {
            PeakGlut4VMAX_ = itr->second;
        }
        if(itr->first.compare("maxGlucoseAbsorptionDuringExercise_") == 0)
        {
            maxGlucoseAbsorptionDuringExercise_ = itr->second;
        }
        if(itr->first.compare("basalGlucoseAbsorbed_") == 0)
        {
            basalGlucoseAbsorbed_ = itr->second;
        }
        if(itr->first.compare("baaToGlutamine_") == 0)
        {
            baaToGlutamine_ = itr->second;
        }
        if(itr->first.compare("glycolysisMin_") == 0)
        {
            glycolysisMin_ = itr->second;
        }
        if(itr->first.compare("glycolysisMax_") == 0)
        {
            glycolysisMax_ = itr->second;
        }
        if(itr->first.compare("glucoseToGlycogen_") == 0)
        {
            glucoseToGlycogen_ = (itr->second);
        }
        if(itr->first.compare("glycogenShareDuringExerciseMean_") == 0)
        {
            glycogenShareDuringExerciseMean_ = itr->second;
        }
    }
}

void Muscles::processTick()
{
    // we assume that all exercise is aerobic exercise.
    
    // molecular mass of glucose is 180.1559 g/mol
    // During glycolysis, 1 molecule of glucose results in 2 molecules of pyruvate (and hence 2 molecules
    // of lactate). So 1mmol/l of lactate corresponds to 0.5mmol/l of glucose. 
    
    static std::poisson_distribution<int> rand__ (100);
    static std::poisson_distribution<int> glycolysisMin__ (1000.0*glycolysisMin_);
    static std::poisson_distribution<int> basalAbsorption__ (1000.0*basalGlucoseAbsorbed_);
    static std::poisson_distribution<int> baaToGlutamine__ (1000.0*baaToGlutamine_);
    static std::poisson_distribution<int> glucoseToGlycogen__ (1000.0*glucoseToGlycogen_);
    
    glucoseAbsorbedPerTick = 0;
    glycogenSynthesizedPerTick = 0;
    glycogenBreakdownPerTick = 0;
    glycogenOxidizedPerTick = 0;
    oxidationPerTick = 0;

    double x; // to hold the random samples
    double currEnergyNeed = body->currentEnergyExpenditure();
    //SimCtl::time_stamp();
    //cout << "HumanBody:: Current Energy Expenditure: " << currEnergyNeed << endl;


    if( body->isExercising() )
    {
        // 10% of energy comes from glucose on average
        //oxidation of 1g of carbs yields 4kcal of energy
	x = (0.9 + (double)(rand__(sim->generator))/1000.0);
        oxidationPerTick = 0.1*x*1000.0*(currEnergyNeed)/4.0; // in milligrams

	double max = maxGlucoseAbsorptionDuringExercise_*(body->bodyWeight);
        if( oxidationPerTick > max )
                oxidationPerTick = max;
	 
	if( glucose >= oxidationPerTick )
		glucose -= oxidationPerTick;
	else
	{
		double g = oxidationPerTick - glucose;
		glucose = 0;
       		body->blood->removeGlucose(g);
		glucoseAbsorbedPerTick += g;
	}
        
	// glycogen share depends on the exercise % VO2Max
	double intensity = body->percentVO2Max;
        double mean = glycogenShareDuringExerciseMean_;
        double stddev = 0.2;
        double glycogenShare = 0.9*0.5*(1 + erf((intensity - mean)/(stddev*sqrt(2))));

	x = (0.9 + (double)(rand__(sim->generator))/1000.0);
        glycogenOxidizedPerTick = glycogenShare*x*1000.0*currEnergyNeed/4.0; // in milligrams
        glycogen -= glycogenOxidizedPerTick;
        glycogenBreakdownPerTick += glycogenOxidizedPerTick;

    	//SimCtl::time_stamp();
        //cout << " Muscles: GlycogenShare " << glycogenShare << endl;

        // do glycolysis
        
        x = (double)(glycolysisMin__(sim->generator));
        x = x*(body->bodyWeight)/1000.0;
            
        if( x > glycolysisMax_*(body->bodyWeight))
             x = glycolysisMax_*(body->bodyWeight);
            
        glycolysisPerTick = x + intensity * ( (glycolysisMax_*(body->bodyWeight)) - x );
        
        glycogen -= glycolysisPerTick;
        glycogenBreakdownPerTick += glycolysisPerTick;
        body->blood->lactate += glycolysisPerTick;

	double kcalgenerated = (oxidationPerTick + glycogenOxidizedPerTick)*0.004 + 
				glycolysisPerTick*0.004/15.0;
	if( kcalgenerated < currEnergyNeed )
        	body->adiposeTissue->consumeFat(currEnergyNeed - kcalgenerated);
    }
    else
    {
    	double insulin_level = body->blood->insulinLevel;

    	// basal absorption
    	x = (double)(basalAbsorption__(sim->generator));
    	x = x*(body->bodyWeight)/1000.0;
    	body->blood->removeGlucose(x);
    	glucoseAbsorbedPerTick = x;
    	glucose += x;

    	// Absorption via GLUT4
    	double bgl = body->blood->getBGL();
    	double glMuscles = glucose/volume_;
    	double diff = bgl-glMuscles;
        
	double scale = body->glut4Impact_;

        if( ((int)(SimCtl::ticks) > body->lastHardExerciseAt + 60) || (bgl < (body->blood->baseBGL())) )
                scale *= insulin_level;

    	double g;
        
    	if( diff > 0 )
    	{
		x = PeakGlut4VMAX_;
                x -= glycogen * (PeakGlut4VMAX_ - Glut4VMAX_)/glycogenMax_;
                x *= (0.9 + (double)(rand__(sim->generator))/1000.0);
                x *= body->bodyWeight;
         	g = scale*x*diff/(diff + Glut4Km_);

         	body->blood->removeGlucose(g);
         	glucoseAbsorbedPerTick += g;
	 	glucose += g;
    	}

    	// glycogen synthesis
        
    	double toGlycogen = (double)(glucoseToGlycogen__(sim->generator)) * (body->bodyWeight)/1000.0;
   
    	if( toGlycogen > glucose )
        	toGlycogen = glucose;
   
   	if( toGlycogen > 0 )
   	{
       		glycogen += toGlycogen;
   	}

   	if( glycogen > glycogenMax_ )
   	{
       		toGlycogen -= glycogen - glycogenMax_;
       		glycogen = glycogenMax_;
   	}
   	glycogenSynthesizedPerTick = toGlycogen;
   	glucose -= toGlycogen;

        // glycolysis

        x = (double)(glycolysisMin__(sim->generator))/1000.0;
        glycolysisPerTick = body->glycolysis(x,glycolysisMax_);

        g = glycolysisPerTick;
        
        if( g <= glucose )
        {
            glucose -= g;
            body->blood->lactate += g;
        }
        else
        {
                body->blood->lactate += glucose;
                glycolysisPerTick = glucose;
                glucose = 0;
/*****************
            g -= glucose;
            body->blood->lactate += glucose;
            glucose = 0;
            
            if( glycogen >= g )
            {
                glycogen -= g;
                body->blood->lactate += g;
		glycogenBreakdownPerTick += g;
            }
            else
            {
                body->blood->lactate += glycogen;
                glycolysisPerTick = glycolysisPerTick -g + glycogen;
		glycogenBreakdownPerTick += glycogen;
                glycogen = 0;
            }
******************/
        }
        
        // oxidation
        //oxidationPerTick = 0.5*(body->blood->insulinLevel)*glucoseAbsorbedPerTick;
        oxidationPerTick = glucose;
        glucose = 0;

        // consume fat for the remaining energy needs during resting state
        double kcalgenerated = oxidationPerTick*0.004 + glycolysisPerTick*0.004/15.0;
		// oxidation produces 15 times more energy than glycolysis 
        if( kcalgenerated < currEnergyNeed )
           body->adiposeTissue->consumeFat(currEnergyNeed-kcalgenerated);
    }
    
    if( glycogen < 0 )
    {
    	SimCtl::time_stamp();
        cout << "Glycogen in muscles went negative\n";
        exit(-1);
    }
    
    //Muscles generate glutamine from branched amino acids.
/*
    x = (double)(baaToGlutamine__(sim->generator));
    x = x/1000.0;
    if( body->blood->branchedAminoAcids > x )
    {
        body->blood->branchedAminoAcids -= x;
        body->blood->glutamine += x;
    }
    else
    {
        body->blood->glutamine += body->blood->branchedAminoAcids;
        body->blood->branchedAminoAcids = 0;
    }
*/
    
    SimCtl::time_stamp();
    cout << " Muscles:: GlucoseAbsorbed " << glucoseAbsorbedPerTick << endl;

    SimCtl::time_stamp();
    cout << " Muscles:: GlycogenSynthesis " << glycogenSynthesizedPerTick << endl;
    SimCtl::time_stamp();
    cout << " Muscles:: GlycogenBreakdown " << glycogenBreakdownPerTick << endl;

    SimCtl::time_stamp();
    cout << " Muscles:: glycogen " << glycogen/1000.0 << endl;
    SimCtl::time_stamp();
    cout << " Muscles:: Oxidation " << oxidationPerTick << endl;
    //SimCtl::time_stamp();
    //cout << " Muscles:: GlycogenOxidation " << glycogenOxidizedPerTick << endl;
    SimCtl::time_stamp();
    cout << " Muscles:: Glycolysis " << glycolysisPerTick << endl;

    //SimCtl::time_stamp();
    //cout << " Muscles:: TotalGlucoseOxidized " << oxidationPerTick + glycogenOxidizedPerTick << endl;

    //totalGlucoseAbsorbed += glucoseAbsorbedPerTick;
    //SimCtl::time_stamp();
   // cout << " Muscles:: totalGlucoseAbsorbed " << totalGlucoseAbsorbed << endl;
}

Muscles::Muscles(HumanBody* myBody)
{
    body = myBody;
    //glycogenMax_ = 0.4*(body->bodyWeight)*15000.0; //40% of body weight is muscles
    // glycogen storing capacity of muscles: 15g/kg of wet muscle weight
    // Frayn Chapter 9
    glycogenMax_ = 500*1000.0;
    glycogen = 500*1000.0;
    volume_ = 250;
    glucose = 0;
    
    
    baaToGlutamine_ = 0;
    
    //GLUT1 is also expressed in skeletal muscle and may play a role in uptake of a glucose
    // at a “basal” rate. Muscle glucose uptake averaged 1.91+- 0.23 micromol/kg/min
    //before glucose ingestion and accounted for 22.0 +- 3.7% of systemic glucose disposal."Splanchnic and Leg
    //Substrate Exchange After Ingestion of a Natural Mixed Meal in Humans". 

    // 1.91 micromol of glucose translates to 1.91*180.1559/1000 mg = 0.344 mg

    //basalGlucoseAbsorbed_ = 1.91 * 0.1801559; //mg per kg body weight per minute 
    basalGlucoseAbsorbed_ = 0; //mg per kg body weight per minute 
    
    //See the explanation in processTick()
    glycolysisMin_ = 0.35 * 1.0 * 0.1801559; //mg per kg per minute
    glycolysisMax_ = 0.9 * 0.35 * 15.0 * 0.1801559; //mg per kg per minute
    
    glucoseToGlycogen_ = 15.0 * 0.1801559; // 15 micromol per kg per minute

    Glut4Km_ = 5*180.1559/10.0; //mg/dl equivalent of 5 mmol/l
    Glut4VMAX_ = 3.5; //mg per kg per minute
    PeakGlut4VMAX_ = 2*3.5; //mg per kg per minute
    
    totalGlucoseAbsorbed = 0;

    maxGlucoseAbsorptionDuringExercise_ = 30.0 * 0.1801559;
    glycogenShareDuringExerciseMean_ = 0.53;
}

