#include <iostream>
#include "Liver.h"
#include "Blood.h"
#include "PortalVein.h"
#include "AdiposeTissue.h"
#include "Muscles.h"
#include "Intestine.h"
#include "math.h"

extern SimCtl* sim;

Liver::Liver(HumanBody* body_)
{
    body = body_;
    glycogen = 100.0*1000.0; // equivalent of 100g of glucose
    glycogenMax_ = 120.0*1000.0; // 120 g of glucose
    
    // Frayn Chapter 9
    
    // 5 micromol per kg per minute = 5*180.1559/1000 mg per kg per minute (ref: Gerich paper)
	// default max glycogen breakdown rate is 5 micromol per kg per minute
    //glycogenToGlucose_ = 5.5 * 0.1801559;
    //glucoseToGlycogen_ = 33.0 * 0.1801559; 
    glycogenToGlucoseInLiver_ = 0.9;
    glucoseToGlycogenInLiver_ = 4.5;

    //Gerich paper: Liver consumes 1 micromol per kg per minute to 16.5 micromol per kg per minute of glucose depending upon post-absorptive/post-prandial state.
    glycolysisMin_ = 0.35 * 0.1801559; //mg per kg per minute
    glycolysisMax_ = 0.9 * 0.35 * 10 * 0.1801559; //mg per kg per minute
    
    glycolysisToLactateFraction_ = 1; // by default glycolysis just generates all lactate
    
    // 1 micromol per kg per minute = 0.1801559 mg per kg per minute
    //double micromol = 0.1801559;
    //gng_ = 0.42 * 4.0 * micromol; 
    gngLiver_ = 0.16; 
    
    //maxLipogenesis_ = 1000.0;
    maxLipogenesis_ = 400.0;

    glucoseToNEFA_ = 0;
    
    fluidVolume_ = 12; //dl; Meyer paper on gluconeogenesis did measurements on liver volume
    glucose = 100*fluidVolume_; // assuming glucose concentration to be 100mg/dl

    Glut2Km_ = 20*180.1559/10.0; // mg/deciliter equal to 20 mmol/l (Frayn Table 2.2.1)
    Glut2VMAX_ = 50; //mg per kg per minute
}

void Liver::processTick()
{
    double x; // to hold the random samples
    
    static std::poisson_distribution<int> rand__ (100);
    static std::poisson_distribution<int> glucoseToGlycogen__ (1000.0*glucoseToGlycogenInLiver_);
    static std::poisson_distribution<int> glycolysisMin__ (1000.0*glycolysisMin_);
    static std::poisson_distribution<int> Glut2VMAX__ (1000.0*Glut2VMAX_);
    
    absorptionPerTick = 0;
    releasePerTick = 0;

    double glInPortalVein = body->portalVein->getConcentration();
    double glInLiver = glucose/fluidVolume_;
    
    if( glInLiver < glInPortalVein )
    {
        double diff = glInPortalVein - glInLiver;
        x = (double)(Glut2VMAX__(sim->generator));
        x *= (body->bodyWeight)/1000.0;
        double g = x * diff/(diff + Glut2Km_);
        
        if( g > body->portalVein->getGlucose() )
        {
            //cout << "Trying to absorb more glucose from portal vein than what is present there! " << g << " " << body->portalVein->getGlucose() << endl;
            g = body->portalVein->getGlucose();
        }
        
        body->portalVein->removeGlucose(g);
        glucose += g;
	absorptionPerTick = g;
        //SimCtl::time_stamp();
        //cout << " Liver absorbs glucose frm portal vein " << g << "mg" << endl;
    }
    //release all portalVein glucose to blood
    body->portalVein->releaseAllGlucose();

    // glycogen synthesis (depends on insulin level and insulin resistance)
    
    double scale = body->insulinImpactOnGlycogenSynthesisInLiver();
    scale *= body->liverGlycogenSynthesisImpact_;

    x = (double)(glucoseToGlycogen__(sim->generator));
    double toGlycogen = scale * x * (body->bodyWeight)/1000.0;
    
    if( toGlycogen > glucose )
        toGlycogen = glucose;
    
    if( toGlycogen > 0 )
    {
	if( glycogen + toGlycogen <= glycogenMax_ )
    		glycogen += toGlycogen;
	else
	{
    		glycogen += toGlycogen;

		if( glycogen - glycogenMax_ > maxLipogenesis_ )
		{
        			body->adiposeTissue->lipogenesis(maxLipogenesis_);
				glycogen -= toGlycogen; // original glycogen
				toGlycogen = maxLipogenesis_ + glycogenMax_ - glycogen;
        			glycogen = glycogenMax_;
    				//SimCtl::time_stamp();
    				//cout << " Liver:: LipogenesisMax " << maxLipogenesis_ << endl;
		}
		else
		{
				double toLipogenesis = glycogen - glycogenMax_;
        			body->adiposeTissue->lipogenesis(toLipogenesis);
        			glycogen = glycogenMax_;
    				//SimCtl::time_stamp();
    				//cout << " Liver:: Lipogenesis " << toLipogenesis << endl;
		}
	}
    }
 
    toGlycogenPerTick = toGlycogen;

    glucose -= toGlycogen;
    
    //cout << "After glycogen synthesis in liver, liver glycogen " << glycogen << " mg, live glucose " << glucose << " mg" << endl;
    
    //Glycolysis. Depends on insulin level. Some of the consumed glucose becomes lactate.
    
    //Gerich paper: Liver consumes 1.65 micomol per kg per minute to 16.5 micomol per kg per minute of glucose depending upon post-absorptive/post-prandial state.
    
    x = (double)(glycolysisMin__(sim->generator))/1000.0;
    double toGlycolysis = body->glycolysis(x,glycolysisMax_);
    
    if( toGlycolysis > glucose)
        toGlycolysis = glucose;
    glucose -= toGlycolysis;
    body->blood->lactate += toGlycolysis*glycolysisToLactateFraction_;
    glycolysisPerTick = toGlycolysis;

    //gluconeogenesis.
    double gng = gngLiver_;
    gng *= body->insulinImpactOnGNG();
    //gng *= (double)(rand__(sim->generator))/100.0;
    gng *= (0.9 + (double)(rand__(sim->generator))/1000.0);
    gng *= body->bodyWeight;

    if( gng > 0 )
    {
        glucose += gng;
    }
    gngPerTick = gng;

    //cout << "After GNG , liver glucose " << glucose << " mg, liver glycogen " << glycogen << " mg, blood glucose " << body->blood->glucose << " mg, blood lactate " << body->blood->lactate << " mg" << endl;
    
    // glycogen breakdown
    double glycogenBreakdown = glycogenToGlucoseInLiver_;
    glycogenBreakdown *= body->insulinImpactOnGlycogenBreakdownInLiver();
    //glycogenBreakdown *= (double)(rand__(sim->generator))/100.0;
    glycogenBreakdown *= (0.9 + (double)(rand__(sim->generator))/1000.0);
    glycogenBreakdown *= body->bodyWeight;

    if( glycogenBreakdown > 0 )
    {
        if( glycogenBreakdown <= glycogen )
        {
                glycogen -= glycogenBreakdown;
                glucose += glycogenBreakdown;
        }
        else
        {
                glucose += glycogen;
                glycogenBreakdown = glycogen;
                glycogen = 0;
        }
    }
    fromGlycogenPerTick = glycogenBreakdown;

/***********************************************************************************************
    // try to maintain glucose homeostasis.
    double glucoseNeeded = body->getGlucoseNeedsOutsideMuscles();
    glucoseNeeded += body->muscles->glucoseAbsorbedPerTick;

    glucoseNeeded -= body->intestine->toPortalVeinPerTick;
    glucoseNeeded -= gngPerTick;
    glucoseNeeded -= body->kidneys->gngPerTick;
    glucoseNeeded -= fromGlycogenPerTick;

    if( glucoseNeeded < 0 )
	glucoseNeeded = 0;

    double fromGlycogen = 0.0;

    if(body->isExercising() )
    {
	double max = maxGlycogenToGlucoseDuringExercise_ * (body->bodyWeight) * (body->maxLiverGlycogenBreakdownDuringExerciseImpact_); 

	fromGlycogen = glucoseNeeded;

	if( glucoseNeeded > max || glucoseNeeded > glycogen )
	{
		double smaller = max;
		if( smaller > glycogen )
			smaller = glycogen;

		fromGlycogen = smaller;
	}
    } 

    if( fromGlycogen > 0 )
    {
    	glycogen -= fromGlycogen;
    	glucose += fromGlycogen;
    }
    fromGlycogenPerTick = fromGlycogen;

   // if no liver glycogen left, invoke GNG again to produce glucose to maintain glucose homeostasis

   if( glycogen == 0 )
   {
   	glucoseNeeded -= fromGlycogen;

    	if(body->isExercising() )
    	{
    		double maxGNGDuringExercise =  (double)(maxGNGDuringExercise__(sim->generator));
    		//SimCtl::time_stamp();
		//cout << " Liver:: Glucose Needed " << glucoseNeeded << " maxGNG " << maxGNGDuringExercise << endl; 
    		//maxGNGDuringExercise *= (body->percentVO2Max)*(body->bodyWeight)/1000.0;
    		maxGNGDuringExercise *= (body->bodyWeight)/1000.0;
		//cout << " Liver:: Glucose Needed " << glucoseNeeded << " maxGNG " << maxGNGDuringExercise << endl; 
   		if( glucoseNeeded > maxGNGDuringExercise - gngPerTick )
			glucoseNeeded = maxGNGDuringExercise - gngPerTick;
    	}

   	if( glucoseNeeded > 0 )
   	{
   		glucose += glucoseNeeded;
    		gngPerTick += glucoseNeeded;
    		//SimCtl::time_stamp();
		//cout << " Liver:: ExtraGNG " << glucoseNeeded << " " << gngPerTick << endl; 
   	}
   }
****************************************************************************************************************/

    //93% of unbranched amino acids in portal vein are retained in Liver, because the leaked amino acids from Intestine consists of 15% branched and 85% unbranched, but after liver consumption the percentage needs to be 70% branched, 30% unbranched. To provide these percentages 93% of unbranched amino acids in portal vein are retained in liver. (From Frayn's book)
    
    body->portalVein->releaseAminoAcids();
    
    glInLiver = glucose/fluidVolume_;
    double bgl = body->blood->getBGL();
 
    if( glInLiver > bgl )
    {
        double diff = glInLiver - bgl;
        x = (double)(Glut2VMAX__(sim->generator));
        x *= (body->bodyWeight)/1000.0;
        double g = x*diff/(diff + Glut2Km_);
        
        if( g > glucose )
        {
            cout << "Releasing more glucose to blood than what is present in liver!\n";
            exit(-1);
        }
        
        glucose -= g;
        body->blood->addGlucose(g);
	releasePerTick = g;
    }
    
    SimCtl::time_stamp();
    cout << " Liver:: ToGlycogen " << toGlycogenPerTick << endl;
    SimCtl::time_stamp();
    cout << " Liver:: glycogen " << glycogen/1000.0 << endl;
    SimCtl::time_stamp();
    cout << " Liver:: FromGlycogen " << fromGlycogenPerTick << endl;
    SimCtl::time_stamp();
    cout << " Liver:: GNG " << gngPerTick << endl;
    //SimCtl::time_stamp();
    //cout << " Liver:: GlucoseProduced " << gngPerTick + fromGlycogenPerTick << endl;

    SimCtl::time_stamp();
    cout << " Liver:: Absorption " << absorptionPerTick << endl;
    SimCtl::time_stamp();
    cout << " Liver:: Glycolysis " << glycolysisPerTick << endl;
    SimCtl::time_stamp();
    cout << " Liver:: Release " << releasePerTick  << "mg, gl " << glucose/fluidVolume_ << endl;
}

void Liver::setParams()
{
    for( ParamSet::iterator itr = body->metabolicParameters[body->bodyState][LIVER].begin();
        itr != body->metabolicParameters[body->bodyState][LIVER].end(); itr++)
    {
        if(itr->first.compare("Glycogen") == 0)
        {
            glycogen = 1000.0 * (itr->second);
        }
        if(itr->first.compare("MaxGlycogen") == 0)
        {
            glycogenMax_ = 1000.0 * (itr->second);
        }
        if(itr->first.compare("fluidVolume_") == 0)
        {
            fluidVolume_ = itr->second;
        }
        if(itr->first.compare("Glut2Km_") == 0)
        {
            Glut2Km_ = itr->second;
        }
        if(itr->first.compare("Glut2VMAX_") == 0)
        {
            Glut2VMAX_ = itr->second;
        }
        if(itr->first.compare("glucoseToGlycogenInLiver_") == 0)
        {
            glucoseToGlycogenInLiver_ = (itr->second); 
        }
        if(itr->first.compare("glycogenToGlucoseInLiver_") == 0)
        {
            glycogenToGlucoseInLiver_ = itr->second;
        }
        if(itr->first.compare("glycolysisMin_") == 0)
        {
            glycolysisMin_ = itr->second;
        }
        if(itr->first.compare("glycolysisMax_") == 0)
        {
            glycolysisMax_ = itr->second;
        }
        if(itr->first.compare("glycolysisToLactateFraction_") == 0)
        {
            glycolysisToLactateFraction_ = itr->second;
        }
        if(itr->first.compare("gngLiver_") == 0)
        {
            gngLiver_ = itr->second;
        }
        if(itr->first.compare("glucoseToNEFA_") == 0)
        {
            glucoseToNEFA_ = itr->second;
        }
        if(itr->first.compare("maxLipogenesis_") == 0)
        {
            maxLipogenesis_ = itr->second;
        }
    }
}

