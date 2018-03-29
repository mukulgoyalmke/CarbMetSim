#include <iostream>
#include "Liver.h"
#include "Blood.h"
#include "PortalVein.h"
#include "AdiposeTissue.h"
#include "math.h"

Liver::Liver(HumanBody* body_)
{
    body = body_;
    glycogen = 100000.0; // equivalent of 100g of glucose
    glycogenMax_ = 120000.0; // 120 g of glucose
    
    // Frayn Chapter 9
    
    // 5 micromol per kg per minute = 5*180.1559/1000 mg per kg per minute = 0.9007795 mg per kg per minute (ref: Gerich paper)
	// default max glycogen breakdown rate is 10 micromol per kg per minute
    glycogenToGlucose_ = 2*0.9007795;
    glucoseToGlycogen_ = glycogenToGlucose_; // for now

    glycogenSynth_Insulin_Mean_ = 0.075;
    glycogenSynth_Insulin_StdDev_ = 0.02;

    //Gerich paper: Liver consumes 1.65 micromol per kg per minute to 16.5 micromol per kg per minute of glucose depending upon post-absorptive/post-prandial state.
    glycolysisMin_ = 0.297; //mg per kg per minute
    glycolysisMax_ = 2.972;
    
    glycolysisToLactateFraction_ = 1; // by default glycolysis just generates all lactate
    
    // 2.5 micromol per kg per minute = 2.5*180.1559/1000 mg per kg per minute = 0.45038975 mg per kg per minute
    // default gng rate is 5 micromol per kg per minute
    gluconeogenesisRate_ = 2.0*0.45038975;
    gngFromLactateRate_ = 9*gluconeogenesisRate_; //by default
    
    glucoseToNEFA_ = 0;
    
    fluidVolume_ = 10; //dl
    glucose = 100*fluidVolume_; // assuming glucose concentration to be 100mg/dl

    Glut2Km_ = 20*180.1559/10.0; // mg/deciliter equal to 20 mmol/l (Frayn Table 2.2.1)
    Glut2VMAX_ = 50; //mg per kg per minute
}

void Liver::processTick()
{
    double baseBGL = body->blood->baseBGL();

    double x; // to hold the random samples
    
    static std::poisson_distribution<int> glycogenToGlucose__ (1000.0*glycogenToGlucose_);
    static std::poisson_distribution<int> glucoseToGlycogen__ (1000.0*glucoseToGlycogen_);
    static std::poisson_distribution<int> glycolysisMin__ (1000.0*glycolysisMin_);
    static std::poisson_distribution<int> gngRate__ (1000.0*gluconeogenesisRate_);
    static std::poisson_distribution<int> gngFromLactateRate__ (1000.0*gngFromLactateRate_);
    static std::poisson_distribution<int> Glut2VMAX__ (1000.0*Glut2VMAX_);
    
    double glInPortalVein = body->portalVein->getConcentration();
    double glInLiver = glucose/fluidVolume_;
    
    if( glInLiver < glInPortalVein )
    {
        double diff = glInPortalVein - glInLiver;
        x = (double)(Glut2VMAX__(SimCtl::myEngine()));
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

    // glycogen synthesis (depends on insulin and glucose level)
    
    glInLiver = glucose/fluidVolume_;
    double scale = 1.0;
    if( glInLiver > baseBGL )
    {
        scale *= glInLiver/baseBGL;
    }
    
    scale *= (1.0 - body->insulinResistance_);
    //scale *= body->blood->insulinLevel;
    //if( body->blood->insulinLevel == 0 ) scale = 0;
    scale *= 0.5*(1 + erf((body->blood->insulinLevel - glycogenSynth_Insulin_Mean_)/(glycogenSynth_Insulin_StdDev_*sqrt(2))));

    x = (double)(glucoseToGlycogen__(SimCtl::myEngine()));
    double toGlycogen = scale * x * (body->bodyWeight)/1000.0;
    
    //cout << "glInLiver " << glInLiver << " baseBGL " << baseBGL << " scale "
    //    << scale<< " toGlycogen " << toGlycogen << endl;

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
    
    //glycogen breakdown (depends on insulin and glucose level)
    
    scale = 1 - (body->blood->insulinLevel)*(1 - (body->insulinResistance_));
    glInLiver = glucose/fluidVolume_;
    
    if( glInLiver > baseBGL )
    {
        scale *= baseBGL/glInLiver;
    }
    
    x = (double)(glycogenToGlucose__(SimCtl::myEngine()));
    double fromGlycogen = scale * x * (body->bodyWeight)/1000.0;
    
    if( fromGlycogen > glycogen )
        fromGlycogen = glycogen;
    
    if( fromGlycogen > 0 )
    {
    	glycogen -= fromGlycogen;
    	glucose += fromGlycogen;
    }
    fromGlycogenPerTick = fromGlycogen;

    //cout << "After glycogen breakdown in liver, liver glycogen " << glycogen << " mg, liver glucose " << glucose << " mg, blood glucose " << body->blood->glucose << " mg, blood lactate " << body->blood->lactate << " mg" << endl;

    
    //Glycolysis. Depends on insulin level. Some of the consumed glucose becomes lactate.
    
    //Gerich paper: Liver consumes 1.65 micomol per kg per minute to 16.5 micomol per kg per minute of glucose depending upon post-absorptive/post-prandial state.
    
    scale = (1.0 - body->insulinResistance_)*(body->blood->insulinLevel);
    
    x = (double)(glycolysisMin__(SimCtl::myEngine()));
    x *= (body->bodyWeight)/1000.0;
    if( x > glycolysisMax_*(body->bodyWeight))
        x = glycolysisMax_*(body->bodyWeight);

    double toGlycolysis = x + scale* ( (glycolysisMax_*(body->bodyWeight)) - x);
    
    if( toGlycolysis > glucose)
        toGlycolysis = glucose;
    glucose -= toGlycolysis;
    body->blood->lactate += toGlycolysis*glycolysisToLactateFraction_;
    glycolysisPerTick = toGlycolysis;

    //SimCtl::time_stamp();
    //cout << " glycolysis in liver " << toGlycolysis << "mg" << endl;
    //cout << "After glycolysis , liver glucose " << glucose << " mg, blood lactate " << body->blood->lactate << " mg" << endl;
    
    //gluconeogenesis. Depends on insulin level and on substrate concentration.
    
    scale = 1 - (body->blood->insulinLevel)*(1 - (body->insulinResistance_));
    x = (double)(gngRate__(SimCtl::myEngine()));
    double gng = x *scale * (body->bodyWeight)/1000.0;
    gng = body->blood->consumeGNGSubstrates(gng);
    if( gng > 0 )
    {
    	glucose += gng;
    	//SimCtl::time_stamp();
    	//cout << " gng in liver " << gng << "mg" << endl;
    }
    gngPerTick = gng;
    
    //Gluconeogenesis will occur even in the presence of high insulin in proportion to lactate concentration. 

    x = (double)(gngFromLactateRate__(SimCtl::myEngine()));
    x *= (body->bodyWeight)/1000.0;
    x = body->blood->gngFromHighLactate(x);
    if( x > 0 )
    {
    	glucose += x;
    	//SimCtl::time_stamp();
    	//cout << " gng in liver from high lactate " << x << "mg" << endl;
    }
    gngPerTick += x;
    
    //cout << "After GNG , liver glucose " << glucose << " mg, liver glycogen " << glycogen << " mg, blood glucose " << body->blood->glucose << " mg, blood lactate " << body->blood->lactate << " mg" << endl;
    
    //BUKET NEW: 93% of unbranched amino acids in portal vein are retained in Liver, because the leaked amino acids from Intestine consists of 15% branched and 85% unbranched, but after liver consumption the percentage needs to be 70% branched, 30% unbranched. To provide these percentages 93% of unbranched amino acids in portal vein are retained in liver. (From Frayn's book)
    
    body->portalVein->releaseAminoAcids();
    
    //6. consume some glucose to form fatty acids (at configured rate that depends on glucose level)

    /*
    if( body->blood->glucose > baseBGL ){
        body->blood->consumeGlucose(glucoseToNEFA_);
    }
    */
    
    glInLiver = glucose/fluidVolume_;
    double bgl = body->blood->getBGL();
    
    releasePerTick = 0;

    if( glInLiver > bgl )
    {
        double diff = glInLiver - bgl;
        x = (double)(Glut2VMAX__(SimCtl::myEngine()));
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
        //SimCtl::time_stamp();
        //cout << " Liver released glucose " << g << "mg to blood" << endl;
    }
    
    SimCtl::time_stamp();
    cout << " Liver:: Absorption " << absorptionPerTick << endl;
    SimCtl::time_stamp();
    cout << " Liver:: ToGlycogen " << toGlycogenPerTick << endl;
    SimCtl::time_stamp();
    cout << " Liver:: FromGlycogen " << fromGlycogenPerTick << endl;
    SimCtl::time_stamp();
    cout << " Liver:: glycogen " << glycogen << endl;
    SimCtl::time_stamp();
    cout << " Liver:: Glycolysis " << glycolysisPerTick << endl;
    SimCtl::time_stamp();
    cout << " Liver:: GNG " << gngPerTick << endl;
    SimCtl::time_stamp();
    cout << " Liver:: Release " << releasePerTick << endl;

//glycogen " << glycogen << "mg, glucose " << glucose << "mg, gl " << glucose/fluidVolume_ << endl;
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
        if(itr->first.compare("gluconeogenesisRate_") == 0)
        {
            gluconeogenesisRate_ = itr->second;
        }
        if(itr->first.compare("gngFromLactateRate_") == 0)
        {
            gngFromLactateRate_ = itr->second;
        }
        if(itr->first.compare("glucoseToNEFA_") == 0)
        {
            glucoseToNEFA_ = itr->second;
        }
    }
}

