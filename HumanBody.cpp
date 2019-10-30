#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include "HumanBody.h"
#include "Stomach.h"
#include "Intestine.h"
#include "PortalVein.h"
#include "Liver.h"
#include "Blood.h"
#include "Heart.h"
#include "Brain.h"
#include "Muscles.h"
#include "AdiposeTissue.h"
#include "Kidneys.h"
#include "SimCtl.h"

using namespace std;

HumanBody::HumanBody()
{
    stomach = new Stomach(this);
    intestine = new Intestine(this);
    portalVein = new PortalVein(this);
    liver = new Liver(this);
    brain =new Brain(this);
    heart =new Heart(this);
    blood = new Blood(this);
    kidneys = new Kidneys(this);
    adiposeTissue = new AdiposeTissue(this);
    muscles = new Muscles(this);
    
    glut4Impact_ = 1.0;
    liverGlycogenBreakdownImpact_ = 6.0;
    liverGlycogenSynthesisImpact_ = 1.0;
    gngImpact_ = 6.0;
    glycolysisMinImpact_ = 1.0;
    glycolysisMaxImpact_ = 1.0;
    excretionKidneysImpact_ = 1.0;

    bodyState = POSTABSORPTIVE_RESTING;
    //bodyWeight = 65; //kg
    // must specify body weight in the params
    fatFraction_ = 0.2;
    
    currExercise = 0;
    
    vo2Max = 0;

    // current energy expenditure in kcal/minute per kg of body weight
    currEnergyExpenditure = 1.0/60.0;
    // energy expenditure in resting state is 1 MET
    
    exerciseOverAt = 0; // when does the current exercise event get over

   	lastHardExerciseAt = -61;

	insulinImpactOnGlycolysis_Mean = 0.5;
	insulinImpactOnGlycolysis_StdDev = 0.2;
	insulinImpactOnGNG_Mean = 0.5;
	insulinImpactOnGNG_StdDev = 0.2;
	insulinImpactGlycogenBreakdownInLiver_Mean = 0.1;
	insulinImpactGlycogenBreakdownInLiver_StdDev = 0.02;
	insulinImpactGlycogenSynthesisInLiver_Mean = 0.5;
	insulinImpactGlycogenSynthesisInLiver_StdDev = 0.2;

        intensityPeakGlucoseProd_ = 0.2;

	resetTotals(false);
}

HumanBody::~HumanBody()
{
    delete stomach;
    delete intestine;
    delete portalVein;
    delete liver;
    delete adiposeTissue;
    delete brain;
    delete muscles;
    delete blood;
    delete heart;
    delete kidneys;
}

double HumanBody::insulinImpactOnGlycolysis()
{
	double insulin_level = blood->insulinLevel;
	double scale = 0.5*(1 + erf((insulin_level - insulinImpactOnGlycolysis_Mean)/(insulinImpactOnGlycolysis_StdDev*sqrt(2))));
	return scale;
}

double HumanBody::insulinImpactOnGNG()
{
	return 1.0;
}

/********************************************
double HumanBody::insulinImpactOnGNG()
{
        double insulin_level = blood->insulinLevel;

        if( gngImpact_ < 1.0 )
        {
                cout << "gngImpact_ less than 1" << endl;
                exit(-1);
        }

        if( blood->baseInsulinLevel_ >= insulinImpactOnGNG_Mean )
        {
                cout << "error configuring baseInsulinLevel and insulinImpactOnGNG" << endl;
                exit(-1);
        }

        if( insulin_level >= blood->baseInsulinLevel_ )
        {
                double scale = 0.5*(1 + erf((insulin_level - insulinImpactOnGNG_Mean)/(insulinImpactOnGNG_StdDev*sqrt(2))));
                return (1.0 - scale);
        }
        else
        {
                return ( gngImpact_ -  insulin_level*(gngImpact_ - 1.0)/(blood->baseInsulinLevel_) );
        }
        //return 1.0;
}
********************************************/

double HumanBody::insulinImpactOnGlycogenBreakdownInLiver()
{
        double insulin_level = blood->insulinLevel;

        if( liverGlycogenBreakdownImpact_ < 1.0 )
        {
                cout << "liverGlycogenBreakdownImpact_ less than 1" << endl;
                exit(-1);
        }

        if( blood->baseInsulinLevel_ >= insulinImpactGlycogenBreakdownInLiver_Mean )
        {
                cout << "error configuring baseInsulinLevel and insulinImpactGlycogenBreakdownInLiver" << endl;
                exit(-1);
        }

        if( insulin_level >= blood->baseInsulinLevel_ )
        {
                double scale = 0.5*(1 + erf((insulin_level - insulinImpactGlycogenBreakdownInLiver_Mean)/(insulinImpactGlycogenBreakdownInLiver_StdDev*sqrt(2))));
                return (1.0 - scale);
        }
        else
        {
                return ( liverGlycogenBreakdownImpact_ -  insulin_level*(liverGlycogenBreakdownImpact_ - 1.0)/(blood->baseInsulinLevel_) );
        }
}

double HumanBody::insulinImpactOnGlycogenSynthesisInLiver()
{
	double insulin_level = blood->insulinLevel;
	double scale = 0.5*(1 + erf((insulin_level - insulinImpactGlycogenSynthesisInLiver_Mean)/(insulinImpactGlycogenSynthesisInLiver_StdDev*sqrt(2))));
	return scale;
}

double HumanBody::glycolysis(double min, double max)
{
	double max_ = max * bodyWeight * glycolysisMaxImpact_;

	double min_ = min * bodyWeight * glycolysisMinImpact_;

    	if( min_ > max_ )
        	min_ = max_;

    	double toGlycolysis = min_ + insulinImpactOnGlycolysis() * ( max_ - min_);
	return toGlycolysis;
}

// returns energy expenditure in kcal/minute
double HumanBody::currentEnergyExpenditure()
{
	return bodyWeight*currEnergyExpenditure;
}

void HumanBody::stomachEmpty()
{
    BodyState oldState = bodyState;
    
    //cout << endl;
    
    //cout << "STOMACH EMPTY " << bodyState << endl;
    
    switch (bodyState)
    {
        case FED_RESTING:
            bodyState = POSTABSORPTIVE_RESTING;
            break;
        case FED_EXERCISING:
            bodyState = POSTABSORPTIVE_EXERCISING;
            break;
        default:
            break;
    }
    
    if( bodyState != oldState)
    {
        //setParams();
        //SimCtl::time_stamp();
        //cout << "Entering State " << bodyState << endl;
    }
}

double HumanBody::getGlucoseNeedsOutsideMuscles()
{
    double x = intestine->glycolysisPerTick + liver->glycolysisPerTick + kidneys->glycolysisPerTick + blood->glycolysisPerTick;
    x += brain->oxidationPerTick + heart->oxidationPerTick;
    return x;
}

void HumanBody::resetTotals(bool print)
{
	if(print)
	{
        	SimCtl::time_stamp();
        	cout << " Totals for the day: " 
			<< totalGlycolysisSoFar << " "
    			<< totalExcretionSoFar << " "
    			<< totalOxidationSoFar << " "
    			<< totalGNGSoFar << " "
    			<< totalLiverGlycogenStorageSoFar << " "
    			<< totalLiverGlycogenBreakdownSoFar << " "
    			<< totalMusclesGlycogenStorageSoFar << " "
    			<< totalMusclesGlycogenBreakdownSoFar << " "
    			<< totalGlucoseFromIntestineSoFar << endl;
	}

	totalGlycolysisSoFar = 0;
    	totalExcretionSoFar = 0;
    	totalOxidationSoFar = 0;
    	totalGNGSoFar = 0;
    	totalLiverGlycogenStorageSoFar = 0;
    	totalLiverGlycogenBreakdownSoFar = 0;
    	totalMusclesGlycogenStorageSoFar = 0;
    	totalMusclesGlycogenBreakdownSoFar = 0;
    	totalGlucoseFromIntestineSoFar = 0;

	dailyCarbs = 0;
}

void HumanBody::processTick()
{
    //Gerich: In terms of whole-body glucose economy, normally approximately 45% of ingested glucose is thought to be
    // converted to glycogen in the liver, 30% is taken up by skeletal muscle and later converted to glycogen, 
    // 15% is taken up by the brain, 5% is taken up by the adipose tissue and 10% is taken up by the kidneys
    
    portalVein->processTick();
    stomach->processTick();
    intestine->processTick();
    liver->processTick();
    adiposeTissue->processTick();
    brain->processTick();
    heart->processTick();
    muscles->processTick();
    kidneys->processTick();
    blood->processTick();
    
    double currBGL = blood->getBGL();

    SimCtl::time_stamp();
    cout << " " << currBGL << " " << (liver->glycogen)/1000.0 << " " << (muscles->glycogen)/1000.0 << endl;
    SimCtl::time_stamp();
    cout << " HumanBody:: BGL " << currBGL << endl;
    //SimCtl::time_stamp();
    //cout << " weight " << bodyWeight << endl;

    double x = intestine->glycolysisPerTick + liver->glycolysisPerTick + muscles->glycolysisPerTick 
		+ kidneys->glycolysisPerTick + blood->glycolysisPerTick;
    totalGlycolysisSoFar += x;

    SimCtl::time_stamp();
    cout << " HumanBody:: TotalGlycolysisPerTick " << x << endl;
    SimCtl::time_stamp();
    cout << " HumanBody:: TotalGlycolysisSoFar " << totalGlycolysisSoFar << endl;

    x = kidneys->gngPerTick + liver->gngPerTick; 
    totalGNGSoFar += x;

    SimCtl::time_stamp();
    cout << " HumanBody:: TotalGNGPerTick " << x << endl;
    SimCtl::time_stamp();
    cout << " HumanBody:: TotalGNGSoFar " << totalGNGSoFar << endl;

    x = brain->oxidationPerTick + heart->oxidationPerTick + muscles->oxidationPerTick;
    totalOxidationSoFar += x;
    SimCtl::time_stamp();
    cout << " HumanBody:: TotalOxidationPerTick " << x << endl;
    SimCtl::time_stamp();
    cout << " HumanBody:: TotalOxidationSoFar " << totalOxidationSoFar << endl;

    SimCtl::time_stamp();
    cout << " HumanBody:: UseOfGlucoseOutsideLiverKidneysMuscles " << blood->glycolysisPerTick + 
		brain->oxidationPerTick + 
		heart->oxidationPerTick + 
		intestine->glycolysisPerTick << endl;
		
    x = liver->toGlycogenPerTick + muscles->glycogenSynthesizedPerTick;
    totalLiverGlycogenStorageSoFar += liver->toGlycogenPerTick;
    totalMusclesGlycogenStorageSoFar += muscles->glycogenSynthesizedPerTick;
    SimCtl::time_stamp();
    cout << " HumanBody:: TotalGlycogenStoragePerTick " << x << endl;
    SimCtl::time_stamp();
    cout << " HumanBody:: TotalGlycogenStorageSoFar " << 
    totalLiverGlycogenStorageSoFar + totalMusclesGlycogenStorageSoFar 
    << endl;

    x = liver->fromGlycogenPerTick + muscles->glycogenBreakdownPerTick;
    totalLiverGlycogenBreakdownSoFar += liver->fromGlycogenPerTick;
    totalMusclesGlycogenBreakdownSoFar += muscles->glycogenBreakdownPerTick;
    SimCtl::time_stamp();
    cout << " HumanBody:: TotalGlycogenBreakdownPerTick " << x << endl;
    SimCtl::time_stamp();
    cout << " HumanBody:: TotalGlycogenBreakdownSoFar " << 
    totalLiverGlycogenBreakdownSoFar + totalMusclesGlycogenBreakdownSoFar 
    << endl;

    x = liver->fromGlycogenPerTick + kidneys->gngPerTick + liver->gngPerTick; 
    totalEndogeneousGlucoseReleaseSoFar += x;
    SimCtl::time_stamp();
    cout << " HumanBody:: TotalEndogeneousGlucoseReleasePerTick " << x << endl;
    SimCtl::time_stamp();
    cout << " HumanBody:: TotalEndogeneousGlucoseReleaseSoFar " << totalEndogeneousGlucoseReleaseSoFar << endl;

    x = intestine->toPortalVeinPerTick + liver->fromGlycogenPerTick + kidneys->gngPerTick + liver->gngPerTick; 
    totalGlucoseReleaseSoFar += x;
    SimCtl::time_stamp();
    cout << " HumanBody:: TotalGlucoseReleasePerTick " << x << endl;
    SimCtl::time_stamp();
    cout << " HumanBody:: TotalGlucoseReleaseSoFar " << totalGlucoseReleaseSoFar << endl;
    
    totalExcretionSoFar += kidneys->excretionPerTick;
    totalGlucoseFromIntestineSoFar += intestine->toPortalVeinPerTick; 

    if( SimCtl::dayOver() )
	resetTotals(true);

    if (bodyState == FED_EXERCISING)
    {
        if( SimCtl::ticks == exerciseOverAt )
        {
            bodyState = FED_RESTING;
            currEnergyExpenditure = 1.0/60.0;
    	    percentVO2Max = 3.5 * 1.0/vo2Max;
            // energy expenditure in resting state is 1 MET
            //setParams();
        }
    }
    
    if (bodyState == POSTABSORPTIVE_EXERCISING)
    {
        if( SimCtl::ticks == exerciseOverAt )
        {
            bodyState = POSTABSORPTIVE_RESTING;
            currEnergyExpenditure = 1.0/60.0;
    	    percentVO2Max = 3.5 * 1.0/vo2Max;
            //setParams();
        }
    }

	if( SimCtl::ticks == 600 )
	{
		tempGNG = totalGNGSoFar;
        	tempGlycolysis = totalGlycolysisSoFar;
        	tempOxidation = totalOxidationSoFar;
        	tempExcretion = kidneys->totalExcretion;
        	tempGlycogenStorage = totalLiverGlycogenStorageSoFar + totalMusclesGlycogenStorageSoFar;
        	tempGlycogenBreakdown = totalLiverGlycogenBreakdownSoFar + totalMusclesGlycogenBreakdownSoFar;

		baseBGL = currBGL;
		peakBGL = currBGL;
	}

	if( SimCtl::ticks > 600 )
	{
		if( peakBGL < currBGL )
			peakBGL = currBGL;
		//cout << peakBGL << endl;
	}

	if( SimCtl::ticks == 960 )
	{
		cout << "Simulation Results:: GNG " << totalGNGSoFar - tempGNG
		<< " glycolysis " << totalGlycolysisSoFar - tempGlycolysis 
        	<< " oxidation " << totalOxidationSoFar - tempOxidation 
        	<< " excretion " << kidneys->totalExcretion - tempExcretion 
        	<< " glycogenStorage " <<  totalLiverGlycogenStorageSoFar + totalMusclesGlycogenStorageSoFar - tempGlycogenStorage 
        	<< " glycogenBreakdown " <<  totalLiverGlycogenBreakdownSoFar + totalMusclesGlycogenBreakdownSoFar - tempGlycogenBreakdown
		<< " baseBGL " << baseBGL 
		<< " peakBGL " << peakBGL << endl;
	}
}

void HumanBody::setParams()
{
    //send new metabolic rates (based on the new state) to each organ
    
    /*Insulin resistance effects:
     
     1) Absorption of glucose by muscles and adipose tissue slows down
     2) Absorption of glucose by liver (to form glycogen) slows down
     3) glycogen breakdown and gluconeogenesis does not slow down even in presence of high insulin
     4) Glycerol release via lipolysis in adipose tissue does not slow down even in presence of high insulin*/
    
    for( ParamSet::iterator itr = metabolicParameters[bodyState][HUMAN_BODY].begin();
        itr != metabolicParameters[bodyState][HUMAN_BODY].end(); itr++)
    {
        if(itr->first.compare("age_") == 0)
        {
            age = itr->second;
        }
        if(itr->first.compare("gender_") == 0)
        {
            gender = itr->second;
        }
        if(itr->first.compare("fitnessLevel_") == 0)
        {
            fitnessLevel = itr->second;
        }
        if(itr->first.compare("glut4Impact_") == 0)
        {
            glut4Impact_ = itr->second;
        }
        if(itr->first.compare("glycolysisMinImpact_") == 0)
        {
            glycolysisMinImpact_ = itr->second;
        }
        if(itr->first.compare("glycolysisMaxImpact_") == 0)
        {
            glycolysisMaxImpact_ = itr->second;
        }
        if(itr->first.compare("excretionKidneysImpact_") == 0)
        {
            excretionKidneysImpact_ = itr->second;
        }
        if(itr->first.compare("liverGlycogenBreakdownImpact_") == 0)
        {
            liverGlycogenBreakdownImpact_ = itr->second;
        }
        if(itr->first.compare("liverGlycogenSynthesisImpact_") == 0)
        {
            liverGlycogenSynthesisImpact_ = itr->second;
        }
        if(itr->first.compare("maxLiverGlycogenBreakdownDuringExerciseImpact_") == 0)
        {
		maxLiverGlycogenBreakdownDuringExerciseImpact_ = itr->second;
        }
        if(itr->first.compare("gngImpact_") == 0)
        {
            gngImpact_ = itr->second;
        }
        if(itr->first.compare("bodyWeight_") == 0)
        {
            bodyWeight = itr->second;
	    adiposeTissue->fat = fatFraction_*bodyWeight*1000.0;
        }
        if(itr->first.compare("insulinImpactOnGlycolysis_Mean") == 0)
        {
            insulinImpactOnGlycolysis_Mean = itr->second;
        }
        if(itr->first.compare("insulinImpactOnGlycolysis_StdDev") == 0)
        {
            insulinImpactOnGlycolysis_StdDev = itr->second;
        }
        if(itr->first.compare("insulinImpactOnGNG_Mean") == 0)
        {
            insulinImpactOnGNG_Mean = itr->second;
        }
        if(itr->first.compare("insulinImpactOnGNG_StdDev") == 0)
        {
            insulinImpactOnGNG_StdDev = itr->second;
        }
        if(itr->first.compare("insulinImpactGlycogenBreakdownInLiver_Mean") == 0)
        {
            insulinImpactGlycogenBreakdownInLiver_Mean = itr->second;
        }
        if(itr->first.compare("insulinImpactGlycogenBreakdownInLiver_StdDev") == 0)
        {
            insulinImpactGlycogenBreakdownInLiver_StdDev = itr->second;
        }
        if(itr->first.compare("insulinImpactGlycogenSynthesisInLiver_Mean") == 0)
        {
            insulinImpactGlycogenSynthesisInLiver_Mean = itr->second;
        }
        if(itr->first.compare("insulinImpactGlycogenSynthesisInLiver_StdDev") == 0)
        {
            insulinImpactGlycogenSynthesisInLiver_StdDev = itr->second;
        }
        if(itr->first.compare("intensityPeakGlucoseProd_") == 0)
        {
            intensityPeakGlucoseProd_ = itr->second;
        }
    }
    
    setVO2Max();

    stomach->setParams();
    intestine->setParams();
    portalVein->setParams();
    liver->setParams();
    adiposeTissue->setParams();
    brain->setParams();
    heart->setParams();
    muscles->setParams();
    blood->setParams();
    kidneys->setParams();
}

void HumanBody::setVO2Max()
{
	if( gender != 0 && gender != 1 )
	{
		cout << "Invalid gender value" << endl;
		exit(-1);
	}

	if( gender == 0 ) // male
	{
		if( age < 20 )
		{
			cout << "Age below 20 not supported." << endl;
			exit(-1);
		}
		else if( age < 30 )
		{
			if( fitnessLevel <= 5 )
				vo2Max = 29.0; // mLO2 per kg per min
			else if( fitnessLevel <= 10 )
				vo2Max = 32.1;
			else if( fitnessLevel <= 25 )
				vo2Max = 40.1;
			else if( fitnessLevel <= 50 )
				vo2Max = 48.0;
			else if( fitnessLevel <= 75 )
				vo2Max = 55.2;
			else if( fitnessLevel <= 90 )
				vo2Max = 61.8;
			else
				vo2Max = 66.3;
		}
		else if( age < 40 )
		{
			if( fitnessLevel <= 5 )
				vo2Max = 27.2; // mLO2 per kg per min
			else if( fitnessLevel <= 10 )
				vo2Max = 30.2;
			else if( fitnessLevel <= 25 )
				vo2Max = 35.9;
			else if( fitnessLevel <= 50 )
				vo2Max = 42.4;
			else if( fitnessLevel <= 75 )
				vo2Max = 49.2;
			else if( fitnessLevel <= 90 )
				vo2Max = 56.5;
			else
				vo2Max = 59.8;
		}
		else if( age < 50 )
		{
			if( fitnessLevel <= 5 )
				vo2Max = 24.2; // mLO2 per kg per min
			else if( fitnessLevel <= 10 )
				vo2Max = 26.8;
			else if( fitnessLevel <= 25 )
				vo2Max = 31.9;
			else if( fitnessLevel <= 50 )
				vo2Max = 37.8;
			else if( fitnessLevel <= 75 )
				vo2Max = 45.0;
			else if( fitnessLevel <= 90 )
				vo2Max = 52.1;
			else
				vo2Max = 55.6;
		}
		else if( age < 60 )
		{
			if( fitnessLevel <= 5 )
				vo2Max = 20.9; // mLO2 per kg per min
			else if( fitnessLevel <= 10 )
				vo2Max = 22.8;
			else if( fitnessLevel <= 25 )
				vo2Max = 27.1;
			else if( fitnessLevel <= 50 )
				vo2Max = 32.6;
			else if( fitnessLevel <= 75 )
				vo2Max = 39.7;
			else if( fitnessLevel <= 90 )
				vo2Max = 45.6;
			else
				vo2Max = 50.7;
		}
		else if( age < 70 )
		{
			if( fitnessLevel <= 5 )
				vo2Max = 17.4; // mLO2 per kg per min
			else if( fitnessLevel <= 10 )
				vo2Max = 19.8;
			else if( fitnessLevel <= 25 )
				vo2Max = 23.7;
			else if( fitnessLevel <= 50 )
				vo2Max = 28.2;
			else if( fitnessLevel <= 75 )
				vo2Max = 34.5;
			else if( fitnessLevel <= 90 )
				vo2Max = 40.3;
			else
				vo2Max = 43.0;
		}
		else if( age < 80 )
		{
			if( fitnessLevel <= 5 )
				vo2Max = 16.3; // mLO2 per kg per min
			else if( fitnessLevel <= 10 )
				vo2Max = 17.1;
			else if( fitnessLevel <= 25 )
				vo2Max = 20.4;
			else if( fitnessLevel <= 50 )
				vo2Max = 24.4;
			else if( fitnessLevel <= 75 )
				vo2Max = 30.4;
			else if( fitnessLevel <= 90 )
				vo2Max = 36.6;
			else
				vo2Max = 39.7;
		}
		else
		{
			cout << "Age 80 and above not supported." << endl;
			exit(-1);
		}
	}

	if( gender == 1 ) // female
	{
		if( age < 20 )
		{
			cout << "Age below 20 not supported." << endl;
			exit(-1);
		}
		else if( age < 30 )
		{
			if( fitnessLevel <= 5 )
				vo2Max = 21.7; // mLO2 per kg per min
			else if( fitnessLevel <= 10 )
				vo2Max = 23.9;
			else if( fitnessLevel <= 25 )
				vo2Max = 30.5;
			else if( fitnessLevel <= 50 )
				vo2Max = 37.6;
			else if( fitnessLevel <= 75 )
				vo2Max = 44.7;
			else if( fitnessLevel <= 90 )
				vo2Max = 51.3;
			else
				vo2Max = 56.0;
		}
		else if( age < 40 )
		{
			if( fitnessLevel <= 5 )
				vo2Max = 19.0; // mLO2 per kg per min
			else if( fitnessLevel <= 10 )
				vo2Max = 20.9;
			else if( fitnessLevel <= 25 )
				vo2Max = 25.3;
			else if( fitnessLevel <= 50 )
				vo2Max = 30.2;
			else if( fitnessLevel <= 75 )
				vo2Max = 36.1;
			else if( fitnessLevel <= 90 )
				vo2Max = 41.4;
			else
				vo2Max = 45.8;
		}
		else if( age < 50 )
		{
			if( fitnessLevel <= 5 )
				vo2Max = 17.0; // mLO2 per kg per min
			else if( fitnessLevel <= 10 )
				vo2Max = 18.8;
			else if( fitnessLevel <= 25 )
				vo2Max = 22.1;
			else if( fitnessLevel <= 50 )
				vo2Max = 26.7;
			else if( fitnessLevel <= 75 )
				vo2Max = 32.4;
			else if( fitnessLevel <= 90 )
				vo2Max = 38.4;
			else
				vo2Max = 41.7;
		}
		else if( age < 60 )
		{
			if( fitnessLevel <= 5 )
				vo2Max = 16.0; // mLO2 per kg per min
			else if( fitnessLevel <= 10 )
				vo2Max = 17.3;
			else if( fitnessLevel <= 25 )
				vo2Max = 19.9;
			else if( fitnessLevel <= 50 )
				vo2Max = 23.4;
			else if( fitnessLevel <= 75 )
				vo2Max = 27.6;
			else if( fitnessLevel <= 90 )
				vo2Max = 32.0;
			else
				vo2Max = 35.9;
		}
		else if( age < 70 )
		{
			if( fitnessLevel <= 5 )
				vo2Max = 13.4; // mLO2 per kg per min
			else if( fitnessLevel <= 10 )
				vo2Max = 14.6;
			else if( fitnessLevel <= 25 )
				vo2Max = 17.2;
			else if( fitnessLevel <= 50 )
				vo2Max = 20.0;
			else if( fitnessLevel <= 75 )
				vo2Max = 23.8;
			else if( fitnessLevel <= 90 )
				vo2Max = 27.0;
			else
				vo2Max = 29.4;
		}
		else if( age < 80 )
		{
			if( fitnessLevel <= 5 )
				vo2Max = 13.1; // mLO2 per kg per min
			else if( fitnessLevel <= 10 )
				vo2Max = 13.6;
			else if( fitnessLevel <= 25 )
				vo2Max = 15.6;
			else if( fitnessLevel <= 50 )
				vo2Max = 18.3;
			else if( fitnessLevel <= 75 )
				vo2Max = 20.8;
			else if( fitnessLevel <= 90 )
				vo2Max = 23.1;
			else
				vo2Max = 24.1;
		}
		else
		{
			cout << "Age 80 and above not supported." << endl;
			exit(-1);
		}
	}
    	percentVO2Max = 3.5 * 1.0/vo2Max;
		// Assuming rest MET is 1.0
}

void HumanBody::processFoodEvent(unsigned foodID, unsigned howmuch)
{
    stomach->addFood(foodID, howmuch);
    
    BodyState oldState = bodyState;
    
    switch (bodyState)
    {
        case POSTABSORPTIVE_RESTING:
            bodyState = FED_RESTING;
            break;
        case POSTABSORPTIVE_EXERCISING:
            bodyState = FED_EXERCISING;
            break;
        default:
            break;
    }
    
    if( bodyState != oldState)
    {
        //setParams();
        //SimCtl::time_stamp();
        //cout << "Entering State " << bodyState << endl;
    }
}

bool HumanBody::isExercising()
{
    if( (bodyState == FED_EXERCISING) || (bodyState == POSTABSORPTIVE_EXERCISING))
        return true;
    else
        return false;
}

void HumanBody::processExerciseEvent(unsigned exerciseID, unsigned duration)
{
    // how much calorie would be consumed per minute for this exercise?
    // where would this much calorie come from?
    
    if( isExercising() )
    {
        SimCtl::time_stamp();
        cout << "Exercise within Exercise!" << endl;
        exit(-1);
    }
    
    
    if( vo2Max == 0 )
    {
	cout << "vo2Max not known" << endl;
        exit(-1);
    }

    percentVO2Max = 3.5 * (exerciseTypes[exerciseID].intensity_)/vo2Max;

    SimCtl::time_stamp();
    cout << " Starting Exercise at " << percentVO2Max << " %VO2Max" << endl;

    if( percentVO2Max > 1.0 )
    {
	cout << "Exercise intensity beyond the capacity of the user" << endl;
	exit(-1);
    }

    currExercise = exerciseID;
    currEnergyExpenditure = (exerciseTypes[exerciseID].intensity_)/60.0;
    // intensity is in METs, where one MET is 1kcal/(kg.hr)
    

    if( bodyState == FED_RESTING )
    {
        bodyState = FED_EXERCISING;
        exerciseOverAt = SimCtl::ticks + duration;

    	if( exerciseTypes[exerciseID].intensity_ >= 6.0 )
        	lastHardExerciseAt = (int)(exerciseOverAt);

        //setParams();
        //SimCtl::time_stamp();
        //cout << "Entering State " << bodyState << endl;
        return;
    }
    
    if( bodyState == POSTABSORPTIVE_RESTING )
    {
        bodyState = POSTABSORPTIVE_EXERCISING;
        exerciseOverAt = SimCtl::ticks + duration;

    	if( exerciseTypes[exerciseID].intensity_ >= 6.0 )
        	lastHardExerciseAt = (int)(exerciseOverAt);

        //setParams();
        //SimCtl::time_stamp();
        //cout << "Entering State " << bodyState << endl;
        return;
    }
        //SimCtl::time_stamp();
        //cout << "Firing Exercise Event " << exerciseID << " for " << duration << " minutes" << endl;
}

void  HumanBody::readExerciseFile(const char * file)
{
    //cout << file <<endl;
    ifstream cfg(file);
    string line;
    char* str = NULL;
    
    if( cfg.is_open() )
    {
        while( getline(cfg,line) )
        {
            //cout << line << endl;
            
            str = new char[line.length() + 1];
            strcpy(str, line.c_str());
            
            char* tok = strtok(str, " ");
            
            unsigned id = (unsigned)atoi(tok);
            tok = strtok(NULL, " ");
            string name(tok);
            tok = strtok(NULL, " ");
            double intensity = atof(tok);
            exerciseTypes[id].exerciseID_ = id;
            exerciseTypes[id].name_ = name;
            exerciseTypes[id].intensity_ = intensity;
            
            delete [] str;
        }
        cfg.close();
    }
    else
    {
        cout << "Error opening " << file << endl;
        exit(1);
    }
}

void  HumanBody::readFoodFile(const char * file)
{
    ifstream fl;
    fl.open(file);
    string line;
    char* str = NULL;
    
    if( fl.is_open() )
    {
    
        while( getline(fl,line) )
        {
            //cout << line << endl;
            
            str = new char[line.length() + 1];
            strcpy(str, line.c_str());
            
            
            char* tok = strtok(str, " ");
            
            unsigned id = (unsigned)atoi(tok);
            tok = strtok(NULL, " ");
            string name(tok);
            tok = strtok(NULL, " ");
            double servingSize = atof(tok);
            tok = strtok(NULL, " ");
            double RAG = atof(tok);
            tok = strtok(NULL, " ");
            double SAG = atof(tok);
            tok = strtok(NULL, " ");
            double protein = atof(tok);
            tok = strtok(NULL, " ");
            double fat = atof(tok);
            
            foodTypes[id].foodID_ = id;
            foodTypes[id].name_ = name;
            foodTypes[id].servingSize_ = servingSize; // in grams
            foodTypes[id].RAG_ = RAG;
            foodTypes[id].SAG_ = SAG;
            foodTypes[id].protein_ = protein;
            foodTypes[id].fat_ = fat;
            
            //cout << "food types: " <<foodTypes[id].name_<< " " << foodTypes[id].protein_<< endl;
            
            delete [] str;
        }
        fl.close();
    }
    else
    {
        cout << "Error opening " << file << endl;
        exit(1);
    }
}

void  HumanBody::readParams(const char * file)
{
    ifstream cfg(file);
    string line;
    char* str = NULL;
    
    if( cfg.is_open() )
    {
        while( getline(cfg,line) )
        {
            
            str = new char[line.length() + 1];
            strcpy(str, line.c_str());
           
   	    //cout << str << endl;
 
            char* tok = strtok(str, " ");
            char* tok2 = NULL;
            
            if(strcmp(tok,"ALL") == 0 )
            {
                tok = strtok(NULL," ");
                if(strcmp(tok,"HUMAN_BODY") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[FED_RESTING][HUMAN_BODY][param] = val;
                    metabolicParameters[FED_EXERCISING][HUMAN_BODY][param] = val;
                    metabolicParameters[POSTABSORPTIVE_RESTING][HUMAN_BODY][param] = val;
                    metabolicParameters[POSTABSORPTIVE_EXERCISING][HUMAN_BODY][param] = val;
                }
                
                if(strcmp(tok,"STOMACH") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[FED_RESTING][STOMACH][param] = val;
                    metabolicParameters[FED_EXERCISING][STOMACH][param] = val;
                    metabolicParameters[POSTABSORPTIVE_RESTING][STOMACH][param] = val;
                    metabolicParameters[POSTABSORPTIVE_EXERCISING][STOMACH][param] = val;
                }
                
                if(strcmp(tok,"INTESTINE") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[FED_RESTING][INTESTINE][param] = val;
                    metabolicParameters[FED_EXERCISING][INTESTINE][param] = val;
                    metabolicParameters[POSTABSORPTIVE_RESTING][INTESTINE][param] = val;
                    metabolicParameters[POSTABSORPTIVE_EXERCISING][INTESTINE][param] = val;
                }
                
                if(strcmp(tok,"PORTAL_VEIN") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[FED_RESTING][PORTAL_VEIN][param] = val;
                    metabolicParameters[FED_EXERCISING][PORTAL_VEIN][param] = val;
                    metabolicParameters[POSTABSORPTIVE_RESTING][PORTAL_VEIN][param] = val;
                    metabolicParameters[POSTABSORPTIVE_EXERCISING][PORTAL_VEIN][param] = val;
                }

                if(strcmp(tok,"LIVER") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[FED_RESTING][LIVER][param] = val;
                    metabolicParameters[FED_EXERCISING][LIVER][param] = val;
                    metabolicParameters[POSTABSORPTIVE_RESTING][LIVER][param] = val;
                    metabolicParameters[POSTABSORPTIVE_EXERCISING][LIVER][param] = val;
                }
                
                if(strcmp(tok,"BLOOD") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[FED_RESTING][BLOOD][param] = val;
                    metabolicParameters[FED_EXERCISING][BLOOD][param] = val;
                    metabolicParameters[POSTABSORPTIVE_RESTING][BLOOD][param] = val;
                    metabolicParameters[POSTABSORPTIVE_EXERCISING][BLOOD][param] = val;
                }
                
                if(strcmp(tok,"MUSCLES") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[FED_RESTING][MUSCLES][param] = val;
                    metabolicParameters[FED_EXERCISING][MUSCLES][param] = val;
                    metabolicParameters[POSTABSORPTIVE_RESTING][MUSCLES][param] = val;
                    metabolicParameters[POSTABSORPTIVE_EXERCISING][MUSCLES][param] = val;
                }
                
                if(strcmp(tok,"BRAIN") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[FED_RESTING][BRAIN][param] = val;
                    metabolicParameters[FED_EXERCISING][BRAIN][param] = val;
                    metabolicParameters[POSTABSORPTIVE_RESTING][BRAIN][param] = val;
                    metabolicParameters[POSTABSORPTIVE_EXERCISING][BRAIN][param] = val;

                }
                
                if(strcmp(tok,"HEART") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[FED_RESTING][HEART][param] = val;
                    metabolicParameters[FED_EXERCISING][HEART][param] = val;
                    metabolicParameters[POSTABSORPTIVE_RESTING][HEART][param] = val;
                    metabolicParameters[POSTABSORPTIVE_EXERCISING][HEART][param] = val;

                }
                
                if(strcmp(tok,"ADIPOSE_TISSUE") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[FED_RESTING][ADIPOSE_TISSUE][param] = val;
                    metabolicParameters[FED_EXERCISING][ADIPOSE_TISSUE][param] = val;
                    metabolicParameters[POSTABSORPTIVE_RESTING][ADIPOSE_TISSUE][param] = val;
                    metabolicParameters[POSTABSORPTIVE_EXERCISING][ADIPOSE_TISSUE][param] = val;

                }
                
                if(strcmp(tok,"KIDNEY") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[FED_RESTING][KIDNEY][param] = val;
                    metabolicParameters[FED_EXERCISING][KIDNEY][param] = val;
                    metabolicParameters[POSTABSORPTIVE_RESTING][KIDNEY][param] = val;
                    metabolicParameters[POSTABSORPTIVE_EXERCISING][KIDNEY][param] = val;
                }
            }
            
            if(strcmp(tok,"FED_RESTING") == 0 )
            {
                tok = strtok(NULL," ");
                if(strcmp(tok,"HUMAN_BODY") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[FED_RESTING][HUMAN_BODY][param] = val;
                    
                }
                
                if(strcmp(tok,"STOMACH") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[FED_RESTING][STOMACH][param] = val;
                }
                
                if(strcmp(tok,"INTESTINE") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[FED_RESTING][INTESTINE][param] = val;
                }
                
                if(strcmp(tok,"PORTAL_VEIN") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[FED_RESTING][PORTAL_VEIN][param] = val;
                }
                
                if(strcmp(tok,"LIVER") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[FED_RESTING][LIVER][param] = val;
                }
                
                if(strcmp(tok,"BLOOD") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[FED_RESTING][BLOOD][param] = val;
                }
                
                if(strcmp(tok,"MUSCLES") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[FED_RESTING][MUSCLES][param] = val;
                }
                
                if(strcmp(tok,"BRAIN") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[FED_RESTING][BRAIN][param] = val;
                }
                
                if(strcmp(tok,"HEART") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[FED_RESTING][HEART][param] = val;
                }
                
                if(strcmp(tok,"ADIPOSE_TISSUE") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[FED_RESTING][ADIPOSE_TISSUE][param] = val;
                }
                
                if(strcmp(tok,"KIDNEY") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[FED_RESTING][KIDNEY][param] = val;
                }
            }
            
            if(strcmp(tok,"FED_EXERCISING") == 0 )
            {
                tok = strtok(NULL," ");
                
                if(strcmp(tok,"HUMAN_BODY") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[FED_EXERCISING][HUMAN_BODY][param] = val;
                }
                
                if(strcmp(tok,"STOMACH") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[FED_EXERCISING][STOMACH][param] = val;
                }
                
                if(strcmp(tok,"INTESTINE") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[FED_EXERCISING][INTESTINE][param] = val;
                }
                
                if(strcmp(tok,"PORTAL_VEIN") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[FED_EXERCISING][PORTAL_VEIN][param] = val;
                }
                
                if(strcmp(tok,"LIVER") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[FED_EXERCISING][LIVER][param] = val;
                }
                
                if(strcmp(tok,"BLOOD") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[FED_EXERCISING][BLOOD][param] = val;
                }
                
                if(strcmp(tok,"MUSCLES") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[FED_EXERCISING][MUSCLES][param] = val;
                }
                
                if(strcmp(tok,"BRAIN") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[FED_EXERCISING][BRAIN][param] = val;
                }
                
                if(strcmp(tok,"HEART") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[FED_EXERCISING][HEART][param] = val;
                }
                
                if(strcmp(tok,"ADIPOSE_TISSUE") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[FED_EXERCISING][ADIPOSE_TISSUE][param] = val;
                }
                
                if(strcmp(tok,"KIDNEY") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[FED_EXERCISING][KIDNEY][param] = val;
                }
            }
            
            if(strcmp(tok,"POSTABSORPTIVE_RESTING") == 0 )
            {
                tok = strtok(NULL," ");
                
                if(strcmp(tok,"HUMAN_BODY") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[POSTABSORPTIVE_RESTING][HUMAN_BODY][param] = val;
                }
                
                if(strcmp(tok,"STOMACH") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[POSTABSORPTIVE_RESTING][STOMACH][param] = val;
                }
                
                if(strcmp(tok,"INTESTINE") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[POSTABSORPTIVE_RESTING][INTESTINE][param] = val;
                }
                
                if(strcmp(tok,"PORTAL_VEIN") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[POSTABSORPTIVE_RESTING][PORTAL_VEIN][param] = val;
                }
                
                if(strcmp(tok,"LIVER") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[POSTABSORPTIVE_RESTING][LIVER][param] = val;
                }
                
                if(strcmp(tok,"BLOOD") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[POSTABSORPTIVE_RESTING][BLOOD][param] = val;
                }
                
                if(strcmp(tok,"MUSCLES") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[POSTABSORPTIVE_RESTING][MUSCLES][param] = val;
                }
                
                if(strcmp(tok,"BRAIN") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[POSTABSORPTIVE_RESTING][BRAIN][param] = val;
                }
                
                if(strcmp(tok,"HEART") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[POSTABSORPTIVE_RESTING][HEART][param] = val;
                }
                
                if(strcmp(tok,"ADIPOSE_TISSUE") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[POSTABSORPTIVE_RESTING][ADIPOSE_TISSUE][param] = val;
                }
                
                if(strcmp(tok,"KIDNEY") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[POSTABSORPTIVE_RESTING][KIDNEY][param] = val;
                }
            }
            
            if(strcmp(tok,"POSTABSORPTIVE_EXERCISING") == 0 )
            {
                tok = strtok(NULL," ");
                
                if(strcmp(tok,"HUMAN_BODY") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[POSTABSORPTIVE_EXERCISING][HUMAN_BODY][param] = val;
                }
                
                if(strcmp(tok,"STOMACH") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[POSTABSORPTIVE_EXERCISING][STOMACH][param] = val;
                }
                
                if(strcmp(tok,"INTESTINE") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[POSTABSORPTIVE_EXERCISING][INTESTINE][param] = val;
                }
                
                if(strcmp(tok,"PORTAL_VEIN") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[POSTABSORPTIVE_EXERCISING][PORTAL_VEIN][param] = val;
                }

                if(strcmp(tok,"LIVER") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[POSTABSORPTIVE_EXERCISING][LIVER][param] = val;
                }
                
                if(strcmp(tok,"BLOOD") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[POSTABSORPTIVE_EXERCISING][BLOOD][param] = val;
                }
                
                if(strcmp(tok,"MUSCLES") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[POSTABSORPTIVE_EXERCISING][MUSCLES][param] = val;
                }
                
                if(strcmp(tok,"BRAIN") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[POSTABSORPTIVE_EXERCISING][BRAIN][param] = val;
                }
                
                if(strcmp(tok,"HEART") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[POSTABSORPTIVE_EXERCISING][HEART][param] = val;
                }
                
                if(strcmp(tok,"ADIPOSE_TISSUE") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[POSTABSORPTIVE_EXERCISING][ADIPOSE_TISSUE][param] = val;
                }
                
                if(strcmp(tok,"KIDNEY") == 0)
                {
                    tok = strtok(NULL, " ");
                    string param(tok);
                    tok2 = strtok(NULL, " ");
                    double val = atof(tok2);
                    metabolicParameters[POSTABSORPTIVE_EXERCISING][KIDNEY][param] = val;
                }
            }
            
            delete [] str;
        }
        cfg.close();
    }
    else
    {
        cout << "Error opening " << file << endl;
        exit(1);
    }
}




