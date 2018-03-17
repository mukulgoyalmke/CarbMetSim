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
    
    insulinResistance_ = 0;
    insulinPeakLevel_ = 1.0;
    bodyState = POSTABSORPTIVE_RESTING;
    bodyWeight = 65; //kg
    fatFraction_ = 0.2;
    adiposeTissue = new AdiposeTissue(this);
    muscles = new Muscles(this);
    
    currExercise = 0;
    
    // current energy expenditure in kcal/minute per kg of body weight
    currEnergyExpenditure = 1.0/60.0;
    // energy expenditure in resting state is 1 MET
    
    exerciseOverAt = 0; // when does the current exercise event get over
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
    
    SimCtl::time_stamp();
    cout << " bgl " << blood->getBGL() << endl;
    SimCtl::time_stamp();
    cout << " weight " << bodyWeight << endl;
    SimCtl::time_stamp();
    cout << " TotalGlycolysis " << intestine->glycolysisPerTick + liver->glycolysisPerTick 
	+ muscles->glycolysisPerTick + kidneys->glycolysisPerTick
	+ blood->glycolysisPerTick << endl;
    SimCtl::time_stamp();
    cout << " TotalGNG " << kidneys->gngPerTick + liver->gngPerTick << endl; 
    SimCtl::time_stamp();
    cout << " TotalOxidation " << brain->oxidationPerTick + heart->oxidationPerTick + 
			muscles->oxidationPerTick <<endl;


    if (bodyState == FED_EXERCISING)
    {
        if( SimCtl::ticks == exerciseOverAt )
        {
            bodyState = FED_RESTING;
            currEnergyExpenditure = 1.0/60.0;
            // energy expenditure in resting state is 1 MET
            //setParams();
            //SimCtl::time_stamp();
            //cout << " HumanBody:: State " << bodyState << endl;
        }
        //return;
    }
    
    if (bodyState == POSTABSORPTIVE_EXERCISING)
    {
        if( SimCtl::ticks == exerciseOverAt )
        {
            bodyState = POSTABSORPTIVE_RESTING;
            currEnergyExpenditure = 1.0/60.0;
            //setParams();
            //SimCtl::time_stamp();
            //cout << " HumanBody:: State " << bodyState << endl;
        }
        //return;
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
        if(itr->first.compare("insulinResistance_") == 0)
        {
            insulinResistance_ = itr->second;
        }
        if(itr->first.compare("insulinPeakLevel_") == 0)
        {
            insulinPeakLevel_ = itr->second;
        }
        if(itr->first.compare("bodyWeight_") == 0)
        {
            bodyWeight = itr->second;
        }
    }
    
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
    
    currExercise = exerciseID;
    currEnergyExpenditure = (exerciseTypes[exerciseID].intensity_)/60.0;
    // intensity is in METs, where one MET is 1kcal/(kg.hr)
    
    if( bodyState == FED_RESTING )
    {
        bodyState = FED_EXERCISING;
        exerciseOverAt = SimCtl::ticks + duration;
        //setParams();
        //SimCtl::time_stamp();
        //cout << "Entering State " << bodyState << endl;
        return;
    }
    
    if( bodyState == POSTABSORPTIVE_RESTING )
    {
        bodyState = POSTABSORPTIVE_EXERCISING;
        exerciseOverAt = SimCtl::ticks + duration;
        //setParams();
        //SimCtl::time_stamp();
        //cout << "Entering State " << bodyState << endl;
        return;
    }
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




