#include "Blood.h"
#include "Liver.h"
#include <stdlib.h>
#include <iostream>
#include "SimCtl.h"
#include <math.h>

extern SimCtl* sim;

Blood::Blood(HumanBody* myBody)
{
    body = myBody;

    //tracking RBCs
    bin0 = 1;
    rbcBirthRate_ = 144.0*60*24; // in millions per day (144 million RBCs take birth every minute)
    glycationProbSlope_ = 0.085/10000.0;
    glycationProbConst_ = 0;
    
    // all contents are in units of milligrams of glucose
    fluidVolume_ = 50.0; // in deciliters
    
    gngSubstrates = 0;
    alanine = 0;
    branchedAminoAcids = 0;
    unbranchedAminoAcids = 0;
    glutamine = 0;
    //baseInsulinLevel_ = 1.0/6.0;
    baseInsulinLevel_ = 0;
    peakInsulinLevel_ = 1.0;
    insulinLevel = baseInsulinLevel_;

/***************
    currIndex = 1;
    for(int i = 0; i <= INSULIN_DELAY; i++ )
	pastInsulinLevels[i] = 0.0;
****************/
    
    //Gerich: insulin dependent: 0.5 to 5 micromol per kg per minute
    glycolysisMin_ = 0.35 * 0.5 * 0.1801559;
    //glycolysisMax_ = 0.35 * 5 * 0.1801559;
    glycolysisMax_ = 0.35 * 2 * 0.1801559;
    
    glycolysisToLactate_ = 1.0;

    baseGlucoseLevel_ = 100; //mg/dl
    glucose = baseGlucoseLevel_ * fluidVolume_;
    highGlucoseLevel_ = 200; //mg/dl
    minGlucoseLevel_ = 40; //mg/dl
    highLactateLevel_ = 4053.51; // mg
    // 9 mmol/l of lactate = 4.5 mmol/l of glucose = 4.5*180.1559*5 mg of glucose = 4053.51mg of glucose
    lactate = 450.39; //mg
    // 1mmol/l of lactate = 0.5mmol/l of glucose = 0.5*180.1559*5 mg of glucose = 450.39 mg of glucose

    // initial number of RBCs
    for(int i = 0; i <= MAXAGE; i++)
    {
        AgeBins[i].RBCs = 0.94*rbcBirthRate_;
        AgeBins[i].glycatedRBCs = 0.06*rbcBirthRate_;
    }
    
    avgBGL = 100.0;

    avgBGLOneDay = 0;
    avgBGLOneDaySum = 0;
    avgBGLOneDayCount = 0;

	totalGlycolysisSoFar = 0;
}

/****************
double Blood::delayedInsulinLevel()
{
    int index = currIndex + INSULIN_DELAY;
    
    if( index > INSULIN_DELAY )
        index -= (INSULIN_DELAY + 1);
    
	return pastInsulinLevels[index];
}
******************/

void Blood::updateRBCs()
{
    // will be called once a day
    
    bin0--;
    
    if( bin0 < 0 )
        bin0 = MAXAGE;
    
    //New RBCs take birth
    AgeBins[bin0].RBCs = rbcBirthRate_;
    AgeBins[bin0].glycatedRBCs = 0;
    
    //std::cout << "New RBCs: " << AgeBins[bin0].RBCs << " bin0 " << bin0 << std::endl;
    
    // Old (100 to 120 days old) RBCs die
    int start_bin = bin0 + HUNDREDDAYS;
    
    if( start_bin > MAXAGE )
        start_bin -= (MAXAGE + 1);
    
    //std::cout << "Old RBCs Die\n";
    
    for(int i = 0; i <= (MAXAGE-HUNDREDDAYS); i++)
    {
        int j = start_bin + i;
        
        if( j < 0 )
        {
            SimCtl::time_stamp();
            cout << " RBC bin value negative " << j << endl;
            exit(-1);
        }
        
        if( j > MAXAGE )
            j -= (MAXAGE + 1);
            
        double kill_rate = ((double)(i))/((double)(MAXAGE-HUNDREDDAYS));
        
        AgeBins[j].RBCs *= (1.0 - kill_rate);
        AgeBins[j].glycatedRBCs *= (1.0 - kill_rate);
        
        //std::cout << "bin: " << j << ", RBCs " << AgeBins[j].RBCs << ", Glycated RBCs " << AgeBins[j].glycatedRBCs << " killrate " << kill_rate << std::endl;
    }
    
    //glycate the RBCs
    double glycation_prob = avgBGLOneDay * glycationProbSlope_ + glycationProbConst_;
    
    //std::cout << "RBCs glycate\n";
    
    for(int i = 0; i <= MAXAGE; i++)
    {
        double newly_glycated = glycation_prob * AgeBins[i].RBCs;
        
        AgeBins[i].RBCs -= newly_glycated;
        AgeBins[i].glycatedRBCs += newly_glycated;
        
        //std::cout << "bin: " << i << ", RBCs " << AgeBins[i].RBCs << ", Glycated RBCs " << AgeBins[i].glycatedRBCs << std::endl;
    }
    
    //SimCtl::time_stamp();
    //std::cout << " New HbA1c: " << currentHbA1c() << std::endl;
}

double Blood::currentHbA1c()
{
    double rbcs = 0;
    double glycated_rbcs = 0;
    
    for(int i = 0; i <= MAXAGE; i++)
    {
        rbcs += AgeBins[i].RBCs;
        rbcs += AgeBins[i].glycatedRBCs;
        glycated_rbcs += AgeBins[i].glycatedRBCs;
    }
    
    if(rbcs == 0)
    {
        std::cerr << "Error in Bloody::currentHbA1c\n";
        exit(1);
    }
    
    return glycated_rbcs/rbcs;
}

void Blood::processTick(){
    
    double x; // to hold the random samples

    static std::poisson_distribution<int> glycolysisMin__ (1000.0*glycolysisMin_);
    
    //RBCs consume about 25mg of glucose every minute and convert it to lactate via glycolysis.
    //Gerich: Glycolysis. Depends on insulin level. Some of the consumed glucose becomes lactate.
    
    x = (double)(glycolysisMin__(sim->generator))/1000.0;
    double toGlycolysis = body->glycolysis(x,glycolysisMax_);

    if( toGlycolysis > glucose)
        toGlycolysis = glucose;
    
    glucose -= toGlycolysis;
    glycolysisPerTick = toGlycolysis;
    body->blood->lactate += glycolysisToLactate_*toGlycolysis;
    
    double bgl = glucose/fluidVolume_;
    
    //update insulin level

    if( bgl >= highGlucoseLevel_)
        insulinLevel = peakInsulinLevel_;
    else
    {
    	insulinLevel = baseInsulinLevel_ + (peakInsulinLevel_ - baseInsulinLevel_)*(bgl - baseGlucoseLevel_)/(highGlucoseLevel_ - baseGlucoseLevel_);

	if (insulinLevel < baseInsulinLevel_ )
		insulinLevel = baseInsulinLevel_;
/*****************

	if( temp >= insulinLevel )
		insulinLevel = temp;
	else
	{
		if( temp <= 3* baseInsulinLevel_ ) 
		{
			insulinLevel -= baseInsulinLevel_/100.0;
			if( insulinLevel < baseInsulinLevel_ )
				insulinLevel = baseInsulinLevel_;
		}
		else
			insulinLevel = temp;
	}
******************/
    }
    
/*****************
    currIndex--;
    
    if( currIndex < 0 )
        currIndex = INSULIN_DELAY;
    
    pastInsulinLevels[currIndex] = insulinLevel;
******************/

    //calculating average bgl during a day
    
    if( avgBGLOneDayCount == ONEDAY )
    {
        avgBGLOneDay = avgBGLOneDaySum/avgBGLOneDayCount;
        avgBGLOneDaySum = 0;
        avgBGLOneDayCount = 0;
        updateRBCs();
        //SimCtl::time_stamp();
        //cout << " Blood::avgBGL " << avgBGLOneDay << endl;
    }
    
    avgBGLOneDaySum += bgl;
    avgBGLOneDayCount++;

    //SimCtl::time_stamp();
    //cout << " Blood:: glycolysis " << glycolysisPerTick << endl;
    totalGlycolysisSoFar += glycolysisPerTick;
    //SimCtl::time_stamp();
    //cout << " Blood:: totalGlycolysis " << totalGlycolysisSoFar << endl;
    //SimCtl::time_stamp();
    //cout << " Blood:: insulinLevel " << insulinLevel << endl;
    //" lactate " << lactate << " glutamine " << glutamine << " alanine " << alanine << " gngsubs " << gngSubstrates << " bAA " << branchedAminoAcids << " uAA " <<  unbranchedAminoAcids << endl;
}

double Blood::consumeGNGSubstrates(double howmuch)
{
/*
    double total = gngSubstrates + lactate + alanine + glutamine;
    
    if( total < howmuch )
    {
        gngSubstrates = 0;
        lactate = 0;
        alanine = 0;
        glutamine = 0;
        return total;
    }

    double factor = (total - howmuch)/total;

    gngSubstrates *= factor;
    lactate *= factor;
    alanine *= factor;
    glutamine *= factor;
*/
    if( lactate < howmuch )
    {
	double total = lactate;
        lactate = 0;
        return total;
    }

    lactate -= howmuch;
    return howmuch;
}

void Blood::setParams()
{
    for( ParamSet::iterator itr = body->metabolicParameters[body->bodyState][BLOOD].begin();
        itr != body->metabolicParameters[body->bodyState][BLOOD].end(); itr++)
    {
        if(itr->first.compare("rbcBirthRate_") == 0)
            rbcBirthRate_= itr->second;
        
        if(itr->first.compare("glycationProbSlope_") == 0)
            glycationProbSlope_= itr->second;

        if(itr->first.compare("glycationProbConst_") == 0)
            glycationProbConst_= itr->second;

        if(itr->first.compare("minGlucoseLevel_") == 0)
             minGlucoseLevel_= itr->second;
    
        if(itr->first.compare("baseGlucoseLevel_") == 0)
	{
             baseGlucoseLevel_= itr->second;
	     glucose = fluidVolume_ * baseGlucoseLevel_; 
	}

        if(itr->first.compare("baseInsulinLevel_") == 0)
	{
             baseInsulinLevel_= itr->second;
	     insulinLevel = baseInsulinLevel_; 
	}
        if(itr->first.compare("peakInsulinLevel_") == 0)
	{
             peakInsulinLevel_= itr->second;
	}

        if(itr->first.compare("highGlucoseLevel_") == 0)
            highGlucoseLevel_= itr->second;
        
        if(itr->first.compare("highLactateLevel_") == 0)
            highLactateLevel_= itr->second;
        
        if(itr->first.compare("glycolysisMin_") == 0)
             glycolysisMin_= itr->second;
        
        if(itr->first.compare("glycolysisMax_") == 0)
            glycolysisMax_= itr->second;
        
        if(itr->first.compare("glycolysisToLactate_") == 0)
            glycolysisToLactate_= itr->second;
    }

    if( highGlucoseLevel_ <= baseGlucoseLevel_ )
    {
	cout << "highGlucoseLevel_ <= baseGlucoseLevel_" << endl;
	exit(-1);
    }
}

void Blood::removeGlucose(double howmuch)
{
    glucose -= howmuch;
    
    //std::cout << "Glucose consumed " << howmuch << " ,glucose left " << glucose << std::endl;
    
    if( getBGL() <= minGlucoseLevel_ )
    {
        SimCtl::time_stamp();
        std::cout << " bgl dips to: " << getBGL() << std::endl;
        exit(-1);
    }
    
}

void Blood::addGlucose(double howmuch)
{
    glucose += howmuch;
    //SimCtl::time_stamp();
    //std::cout << " BGL: " << getBGL() << std::endl;
}

double Blood::gngFromHighLactate(double rate_)
{
    //Gluconeogenesis will occur even in the presence of high insulin in proportion to lactate concentration. High lactate concentration (e.g. due to high glycolytic activity) would cause gluconeogenesis to happen even if insulin concentration is high. But then Gluconeogenesis would contribute to glycogen store of the liver (rather than generating glucose).
    // rate_ is in units of mg per kg per minute
    
    double x = rate_ * lactate/highLactateLevel_;
    
    if( x > lactate )
        x = lactate;
    
    lactate -= x;
    return x;
}




    
