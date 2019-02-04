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
    glycogen = 100000.0; // equivalent of 100g of glucose
    glycogenMax_ = 120000.0; // 120 g of glucose
    
    // Frayn Chapter 9
    
    // 5 micromol per kg per minute = 5*180.1559/1000 mg per kg per minute (ref: Gerich paper)
	// default max glycogen breakdown rate is 5 micromol per kg per minute
    glycogenToGlucose_ = 5.5 * 0.1801559;
    glucoseToGlycogen_ = 33.0 * 0.1801559; 
    maxGlycogenToGlucoseDuringExercise_ = 30.0 * 0.1801559;

    //Gerich paper: Liver consumes 1 micromol per kg per minute to 16.5 micromol per kg per minute of glucose depending upon post-absorptive/post-prandial state.
    glycolysisMin_ = 0.35 * 0.1801559; //mg per kg per minute
    glycolysisMax_ = 0.35 * 10 * 0.1801559; //mg per kg per minute
    
    glycolysisToLactateFraction_ = 1; // by default glycolysis just generates all lactate
    
    // 1 micromol per kg per minute = 0.1801559 mg per kg per minute
    double micromol = 0.1801559;
    gngFromLactate_ = 0.42 * 2.0 * micromol; 
    gngFromGlycerol_ = 0.42 * 0.5 * micromol; 
    gngFromGlutamine_ = 0.42 * 0.5 * micromol; 
    gngFromAlanine_ = 0.42 * 1.0 * micromol; 
    
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
    static std::poisson_distribution<int> glycogenToGlucose__ (1000.0*glycogenToGlucose_);
    static std::poisson_distribution<int> glucoseToGlycogen__ (1000.0*glucoseToGlycogen_);
    static std::poisson_distribution<int> glycolysisMin__ (1000.0*glycolysisMin_);
    static std::poisson_distribution<int> gngFromLactate__ (1000.0*gngFromLactate_);
    static std::poisson_distribution<int> gngFromGlycerol__ (1000.0*gngFromGlycerol_);
    static std::poisson_distribution<int> gngFromGlutamine__ (1000.0*gngFromGlutamine_);
    static std::poisson_distribution<int> gngFromAlanine__ (1000.0*gngFromAlanine_);
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
    	glycogen += toGlycogen;
    }
 
    toGlycogenPerTick = toGlycogen;

    if( glycogen > glycogenMax_ )
    {
    //if the liver cannot store any more glycogen, we assume that this glucose (which would have been stored as glycogen) is converted to fat.
        //SimCtl::time_stamp();
        //cout << " glucose consumed for Lipogenesis in liver " << glycogen - glycogenMax_ << "mg" << endl;
        body->adiposeTissue->lipogenesis(glycogen - glycogenMax_);
        glycogen = glycogenMax_;
    }
    
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
 
    scale = body->insulinImpactOnGNG();
    // from non-lactate sources
    double gng =  (double)(gngFromGlycerol__(sim->generator))
    		+ (double)(gngFromGlutamine__(sim->generator))
    		+ (double)(gngFromAlanine__(sim->generator));
    gng *= scale * (body->gngImpact_) * (body->bodyWeight)/1000.0;
    if( gng > 0 )
    {
    	glucose += gng;
    }
    gngPerTick = gng;

    // from lactate
    gng = (double)(gngFromLactate__(sim->generator));
    gng *= scale * (body->gngImpact_) * (body->bodyWeight)/1000.0;
    gng = body->blood->consumeGNGSubstrates(gng);
    if( gng > 0 )
    {
    	glucose += gng;
    }
    gngPerTick += gng;

/*******************************
    //Gluconeogenesis will occur even in the presence of high insulin in proportion to lactate concentration. 
    x = (double)(gngFromHighLactate__(sim->generator));
    x *= (body->bodyWeight)/1000.0;
    x = body->blood->gngFromHighLactate(x);
    if( x > 0 )
    {
    	glucose += x;
    	//SimCtl::time_stamp();
    	//cout << " gng in liver from high lactate " << x << "mg" << endl;
    }
    gngPerTick += x;
**********************************/

    //cout << "After GNG , liver glucose " << glucose << " mg, liver glycogen " << glycogen << " mg, blood glucose " << body->blood->glucose << " mg, blood lactate " << body->blood->lactate << " mg" << endl;
    
    // glycogen breakdown
    double fromGlycogen = 0.0;

    if(body->isExercising() )
    {
	// try to maintain glucose homeostasis.
	fromGlycogen = body->getGlucoseNeedsOutsideMuscles();
	fromGlycogen += body->muscles->glucoseAbsorbedPerTick;

	fromGlycogen -= body->intestine->toPortalVeinPerTick;
	fromGlycogen -= gngPerTick;
	fromGlycogen -= body->kidneys->gngPerTick;

	if( fromGlycogen < 0 )
		fromGlycogen = 0;

	double max = maxGlycogenToGlucoseDuringExercise_*(body->bodyWeight)*(body->maxLiverGlycogenBreakdownDuringExerciseImpact_); 
	if( fromGlycogen > max )
		fromGlycogen = max; 
	
	if( fromGlycogen > glycogen )
		body->stopExercise();
    } 
    else
    {
    	//glycogen breakdown (depends on insulin level and insulin resistance)
    
    	scale = body->liverGlycogenBreakdownImpact_;
    	scale *= body->insulinImpactOnGlycogenBreakdownInLiver();
    	x = (double)(glycogenToGlucose__(sim->generator));
    	fromGlycogen = scale * x * (body->bodyWeight)/1000.0;
    }

    if( fromGlycogen > glycogen )
        fromGlycogen = glycogen;
    
    if( fromGlycogen > 0 )
    {
    	glycogen -= fromGlycogen;
    	glucose += fromGlycogen;
    }
    fromGlycogenPerTick = fromGlycogen;

    // if no glycogen is left

    double bgl = body->blood->getBGL();

    //SimCtl::time_stamp();
    //cout << "howdy! bgl " << bgl << " baseBGL " << body->blood->baseBGL() << " glycogen " << glycogen << endl;

    if( (bgl < body->blood->baseBGL()) && (glycogen <= 0.1) && !(body->isExercising()) )
    {
	// invoke GNG to produce glucose to maintain bgl at the base level

	double glucoseNeeded = body->getGlucoseNeedsOutsideMuscles();
	glucoseNeeded += body->muscles->glucoseAbsorbedPerTick;

	glucoseNeeded -= body->intestine->toPortalVeinPerTick;
	glucoseNeeded -= gngPerTick;
	glucoseNeeded -= body->kidneys->gngPerTick;

	if( glucoseNeeded > 0 )
	{
    		glucose += glucoseNeeded;
    		gngPerTick += glucoseNeeded;
	}
    }

    //BUKET NEW: 93% of unbranched amino acids in portal vein are retained in Liver, because the leaked amino acids from Intestine consists of 15% branched and 85% unbranched, but after liver consumption the percentage needs to be 70% branched, 30% unbranched. To provide these percentages 93% of unbranched amino acids in portal vein are retained in liver. (From Frayn's book)
    
    body->portalVein->releaseAminoAcids();
    
    //6. consume some glucose to form fatty acids (at configured rate that depends on glucose level)

    /*
    if( body->blood->glucose > baseBGL ){
        body->blood->consumeGlucose(glucoseToNEFA_);
    }
    */
    
    glInLiver = glucose/fluidVolume_;
    
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
    
/*
    SimCtl::time_stamp();
    cout << " Liver:: ToGlycogen " << toGlycogenPerTick << endl;
    SimCtl::time_stamp();
    cout << " Liver:: FromGlycogen " << fromGlycogenPerTick << endl;
    SimCtl::time_stamp();
    cout << " Liver:: glycogen " << glycogen/1000.0 << endl;
    SimCtl::time_stamp();
    cout << " Liver:: Absorption " << absorptionPerTick << endl;
    SimCtl::time_stamp();
    cout << " Liver:: Glycolysis " << glycolysisPerTick << endl;
    SimCtl::time_stamp();
    cout << " Liver:: GNG " << gngPerTick << endl;
    SimCtl::time_stamp();
    cout << " Liver:: Release " << releasePerTick  << "mg, gl " << glucose/fluidVolume_ << endl;
*/

//glycogen " << glycogen << "mg, glucose " << glucose 
}

void Liver::setParams()
{
    for( ParamSet::iterator itr = body->metabolicParameters[body->bodyState][LIVER].begin();
        itr != body->metabolicParameters[body->bodyState][LIVER].end(); itr++)
    {
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
        if(itr->first.compare("glucoseToGlycogen_") == 0)
        {
            glucoseToGlycogen_ = itr->second;
        }
        if(itr->first.compare("glycogenToGlucose_") == 0)
        {
            glycogenToGlucose_ = itr->second;
        }
        if(itr->first.compare("maxGlycogenToGlucoseDuringExercise_") == 0)
        {
            maxGlycogenToGlucoseDuringExercise_ = itr->second;
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
        if(itr->first.compare("gngFromLactate_") == 0)
        {
            gngFromLactate_ = itr->second;
        }
        if(itr->first.compare("glucoseToNEFA_") == 0)
        {
            glucoseToNEFA_ = itr->second;
        }
    }
}

