#include <iostream>
#include "Muscles.h"
#include "Blood.h"
#include "AdiposeTissue.h"

void Muscles::setParams()
{
    for( ParamSet::iterator itr = body->metabolicParameters[body->bodyState][MUSCLES].begin();
        itr != body->metabolicParameters[body->bodyState][MUSCLES].end(); itr++)
    {
        if(itr->first.compare("Glut4Km_") == 0)
        {
            Glut4Km_ = itr->second;
        }
        if(itr->first.compare("Glut4VMAX_") == 0)
        {
            Glut4VMAX_ = itr->second;
        }
        if(itr->first.compare("basalGlucoseAbsorbed_") == 0)
        {
            basalGlucoseAbsorbed_ = itr->second;
        }
        if(itr->first.compare("glucoseOxidationFraction_") == 0)
        {
            glucoseOxidationFraction_ = itr->second;
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
    }
}

void Muscles::processTick()
{
    // we assume that all exercise is aerobic exercise.
    
    //Frayn: The major fuels used in aerobic exercise vary with the intensity of the exercise and
    //with the duration. In relatively light exercise most of the energy required comes from
    //non-esterified fatty acids delivered from adipose tissue. At higher intensities, carbohydrate
    //tends to predominate early on, fat becoming more important later as glycogen
    //stores are depleted. As we have seen several times, the amount of glucose present in
    //the circulation and the extracellular fluid is small, and cannot be depleted without
    //harmful effects. Therefore, the carbohydrate used during endurance exercise comes
    //from glycogen stores, both in exercising skeletal muscle and in the liver. In principle,
    //it might also come from gluconeogenesis: exercising muscles always produce some
    //lactic acid, even in aerobic exercise, and this should be a good substrate for hepatic
    //  gluconeogenesis. The use of different fuels at
    // different intensities of exercise is illustrated in Figure 9.9.
    
    //Fig 9.9: For high intensity exercise, about 10% of energy required comes from plasma glucose
    // and 30% comes from muscle glycogen. (Abt 40% comes from plasma NEFA and remaining from muscle
    // Tg). For low intensity exercise, abt 10% energy comes from plasma glucose and remaining mostly
    //from plasma NEFA (some comes from muscle Tg).
    
    //Absorb glucose via GLUT4 when the insulin concentration is high and/or when the person is 
    //exercising. The absorbed glucose is oxidized or used for glycogen synthesis (if the person is not
    // exercising) or glycolysis (if the person is doing anaerobic exercising) to form lactate.
    
    //the glycolysis in muscles (to form lactate) takes place
    //at a basal rate (which causes the lactate concentration in blood of roughly 1mmol/l) . For light 
    //exercise, the lactate concentration (and hence the glycolysis flux) does not increase. But as the 
    //intensity of the exercise increases further, the lactate concentration increases, presumably 
    //because glycolysis flux increases. At peak exercise intensity, the lactate concentration can be as high 
    //as 9mmol/l.
    
    //oxidation of 1 g of carbs yields 4kcal of energy
    // molecular mass of glucose is 180.1559 g/mol
    // During glycolysis, 1 molecule of glucose results in 2 molecules of pyruvate (and hence 2 molecules
    // of lactate). So 1mmol/l of lactate corresponds to 0.5mmol/l of glucose. We assume that this is basal 
    //level of glycolysis. We assume that the level of glycolysis increases linearly with the exercise intensity
    // peaking to 9mmol/l of lactate production (equivalent to 4.5mmol/l of glucose consumption) when exercise 
    //intensity is 18 METs (marathon).
    
    //Assuming that there is 5l of blood in human body, 0.5mmol/l of glucose translates to 0.5*5*180.1559mg = 450mg
    // and 4.5mmol/l of glucose translates to 4053mg of glucose.
    
    //We need to play with glycolysis and gluconeogenesis numbers so that we achieve these balances for
    // lactate concentration. Here we assume that the glcolysis flux increases linearly with exercise intensity.
    
    //Gerich paper says that 2.22 micromol (per kg per minute) of glucose is consumed by muscles in post-absorptive state
    // (the number is abt 20 micromol per kg per minute in post-prandial state (insulin dependent number)). 
    //For now, we will assume that all this glucose is consumed via glycolysis.
    //20 micromol per kg per minute is 20*180.1559/1000 mg = 3.603 mg.
    
    static std::poisson_distribution<int> rand__ (100);
    static std::poisson_distribution<int> glycolysisMin__ (1000.0*glycolysisMin_);
    static std::poisson_distribution<int> basalAbsorption__ (1000.0*basalGlucoseAbsorbed_);
    static std::poisson_distribution<int> Glut4VMAX__ (1000.0*Glut4VMAX_);
    static std::poisson_distribution<int> baaToGlutamine__ (1000.0*baaToGlutamine_);
    
    glucoseAbsorbedPerTick = 0;
    glycogenSynthesizedPerTick = 0;
    glycogenBreakdownPerTick = 0;
    glycogenOxidizedPerTick = 0;
    oxidationPerTick = 0;

    double x; // to hold the random samples
    double currEnergyNeed = body->currentEnergyExpenditure();

    if( body->isExercising() )
    {
        // 10% of energy comes from glucose on average
        //oxidation of 1g of carbs yields 4kcal of energy
        x = (double)(rand__(SimCtl::myEngine()));
        oxidationPerTick = 0.1*(x/100.0)*1000.0*(currEnergyNeed)/4.0; // in milligrams
	if( glucose >= oxidationPerTick )
		glucose -= oxidationPerTick;
	else
	{
		double g = oxidationPerTick - glucose;
		glucose = 0;
       		body->blood->removeGlucose(g);
		glucoseAbsorbedPerTick += g;
	}
        
        double glycogenShare;
        double intensity = body->exerciseTypes[body->currExercise].intensity_;
        if( intensity >= 6.0 )
        {
            glycogenShare = 0.3; // for MET 6 and above, 30% of energy comes from glycogen
        }
        else
        {
                if( intensity < 3.0 )
                {
                    glycogenShare = 0;
                }
                else
                {
                    glycogenShare = 0.3*(intensity - 3.0 )/3.0;
                }
        }
        x = (double)(rand__(SimCtl::myEngine()));
        glycogenOxidizedPerTick = glycogenShare*(x/100.0)*1000.0*currEnergyNeed/4.0; // in milligrams
        glycogen -= glycogenOxidizedPerTick;
        glycogenBreakdownPerTick += glycogenOxidizedPerTick;

        // do glycolysis
        
        if( intensity < 18.0 )
        {
            x = (double)(glycolysisMin__(SimCtl::myEngine()));
            x = x*(body->bodyWeight)/1000.0;
            
            if( x > glycolysisMax_*(body->bodyWeight))
                x = glycolysisMax_*(body->bodyWeight);
            
            glycolysisPerTick = x + ((intensity-1.0)/17.0)* ( (glycolysisMax_*(body->bodyWeight)) - x);
        }
        else
            glycolysisPerTick = glycolysisMax_*(body->bodyWeight);
        
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
	// basal absorption
        x = (double)(basalAbsorption__(SimCtl::myEngine()));
        x = x*(body->bodyWeight)/1000.0;
        
        body->blood->removeGlucose(x);
	glucoseAbsorbedPerTick = x;
	glucose += x;

        // Absorption via GLUT4
        
        double bgl = body->blood->getBGL();
        double glMuscles = glucose/volume_;
        double diff = bgl-glMuscles;
        
        double scale = (1.0 - body->insulinResistance_)*(body->blood->insulinLevel);
        double g;
        
        if( diff > 0 )
        {
            x = (double)(Glut4VMAX__(SimCtl::myEngine()));
            x = x*(body->bodyWeight)/1000.0;
            g = scale*x*diff/(diff + Glut4Km_);

            body->blood->removeGlucose(g);
	    glucoseAbsorbedPerTick += g;
	    glucose += g;
        }

        // glycolysis
        //Gerich paper says that 2.22 micromol (per kg per minute) of glucose is consumed by muscles in
        // post-absorptive state (the number is abt 20 micromol per kg per minute in post-prandial state
        // (insulin dependent number)). For now, we will assume that all this glucose is consumed via
        // glycolysis.

        scale = (1.0 - body->insulinResistance_)*(body->blood->insulinLevel);
        
        x = (double)(glycolysisMin__(SimCtl::myEngine()));
        x = x*(body->bodyWeight)/1000.0;
        
        if( x > glycolysisMax_*(body->bodyWeight))
            x = glycolysisMax_*(body->bodyWeight);
        
        glycolysisPerTick = x + scale* ( (glycolysisMax_*(body->bodyWeight)) - x);
        
        g = glycolysisPerTick;
        
        if( g <= glucose )
        {
            glucose -= g;
            body->blood->lactate += g;
        }
        else
        {
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
        }
        
	// oxidation
	oxidationPerTick = 0.5*glucose;
	glucose *= 0.5;

	// glycogen synthesis
        if( glucose > 0 )
        {
        	//g = (1.0 - body->insulinResistance_)*glucose;
		g = glucose;

		if( glycogen + g > glycogenMax_ )
			g = glycogenMax_ - glycogen;
		
        	glycogen += g;
            	glycogenSynthesizedPerTick += g;
		glucose -= g; 
        }
        
        // consume fat for the remaining energy needs during resting state
        double kcalgenerated = oxidationPerTick*0.004 + glycolysisPerTick*0.004/15.0;
		// oxidation produces 15 times more energy than glycolysis 
        if( kcalgenerated < currEnergyNeed )
           body->adiposeTissue->consumeFat(currEnergyNeed-kcalgenerated);
    }
    
    if( glycogen < 0 )
    {
    	SimCtl::time_stamp();
        cout << "Glycogen went negative\n";
        exit(-1);
    }
    
    //Muscles generate glutamine from branched amino acids.
/*
    x = (double)(baaToGlutamine__(SimCtl::myEngine()));
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
    cout << " Muscles:: glycogen " << glycogen << endl;
    SimCtl::time_stamp();
    cout << " Muscles:: Oxidation " << oxidationPerTick << endl;
    SimCtl::time_stamp();
    cout << " Muscles:: GlycogenOxidation " << glycogenOxidizedPerTick << endl;
    SimCtl::time_stamp();
    cout << " Muscles:: Glycolysis " << glycolysisPerTick << endl;
    SimCtl::time_stamp();
    cout << " Muscles:: Glucose " << glucose << endl;
}

Muscles::Muscles(HumanBody* myBody)
{
    body = myBody;
    glycogenMax_ = 0.4*(body->bodyWeight)*15000.0; //40% of body weight is muscles
    // glycogen storing capacity of muscles: 15g/kg of wet muscle weight
    // Frayn Chapter 9
    glycogen = glycogenMax_;
    glucose = 0;
    volume_ = 10;
    
    
    baaToGlutamine_ = 0;
    
    //GLUT1 is also expressed in skeletal muscle and may play a role in uptake of a glucose
    // at a “basal” rate. Muscle glucose uptake averaged 1.91+- 0.23 micromol/kg/min
    //before glucose ingestion and accounted for 22.0 +- 3.7% of systemic glucose disposal."Splanchnic and Leg
    //Substrate Exchange After Ingestion of a Natural Mixed Meal in Humans". 

    // 1.91 micromol of glucose translates to 1.91*180.1559/1000 mg = 0.344 mg

    basalGlucoseAbsorbed_ = 0.344; //mg per kg body weight per minute 
    
    //See the explanation in processTick()
    glycolysisMin_ = 0.4; //mg per kg per minute
    // 2.22 micromol per kg per minute = 2.22*180.1559/1000 mg per kg per minute = 0.4 mg per per minute
    glycolysisMax_ = 9*glycolysisMin_; //mg per kg per minute
    
    Glut4Km_ = 5*180.1559/10.0; //mg/dl equivalent of 5 mmol/l
    Glut4VMAX_ = 20.0; //mg per kg per minute
    
    glucoseOxidationFraction_ = 0.5;
    //15% of the oral glucose taken up by muscle (2.5±0.9 g) was released as lactate, alanine,
    //or pyruvate; 50% (8.9±1.4 g) was oxidized, and 35% (6.4±2.3 g) was available for storage.
    //(source: Skeletal Muscle Glycolysis, Oxidation, and Storage of an Oral Glucose Load, Kelley et.al.)
}

