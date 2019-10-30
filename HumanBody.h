#ifndef HUMAN_BODY_H
#define HUMAN_BODY_H

#include <map>
#include "SimCtl.h"

struct FoodType {
    unsigned foodID_;
    string name_;
    double servingSize_; // in grams
    double RAG_; // rapidly available glucose (in grams)
    double SAG_; // slowly available glucose (in grams)    
    double protein_; // in grams
    double fat_; // in grams
};

struct ExerciseType {
    unsigned exerciseID_;
    string name_;
    double intensity_; //intensity in METs
};


enum BodyState {FED_RESTING = 0, FED_EXERCISING, POSTABSORPTIVE_RESTING, POSTABSORPTIVE_EXERCISING};

enum BodyOrgan {HUMAN_BODY = 0, STOMACH, INTESTINE, PORTAL_VEIN, LIVER, BLOOD, MUSCLES, BRAIN, HEART, ADIPOSE_TISSUE, KIDNEY};

typedef map<string, double> ParamSet;

class Stomach;
class Intestine;
class PortalVein;
class Liver;
class AdiposeTissue;
class Brain;
class Muscles;
class Blood;
class Heart;
class Kidneys;

class HumanBody {
public:
    map< unsigned, FoodType> foodTypes;
    map<unsigned, ExerciseType> exerciseTypes;
    
    map<BodyState, map<BodyOrgan, ParamSet > > metabolicParameters;
    void setParams(); // send organs their new rates when the state changes
    
    BodyState bodyState;

    double glut4Impact_;
    double liverGlycogenSynthesisImpact_;
    double maxLiverGlycogenBreakdownDuringExerciseImpact_;
    double glycolysisMinImpact_;
    double glycolysisMaxImpact_;
    double excretionKidneysImpact_;

    void setVO2Max();
    int age; // in years
    int gender; // 0 male, 1 female
    int fitnessLevel; // between 0 and 100
    double vo2Max; // estimated from age, gender and fitnessLevel
    double percentVO2Max; // for the current exercise 

    double bodyWeight;
    double fatFraction_;
    unsigned currExercise;
    double currEnergyExpenditure; // current energy expenditure in kcal/minute per kg of body weight
    unsigned exerciseOverAt; // when does the current exercise event get over
    int lastHardExerciseAt; // when was the last "hard" exercise
    
    Stomach* stomach;
    Intestine* intestine;
    PortalVein* portalVein;
    Liver* liver;
    AdiposeTissue* adiposeTissue;
    Brain* brain;
    Muscles* muscles;
    Blood* blood;
    Heart* heart;
    Kidneys* kidneys;
    
    HumanBody();
    ~HumanBody();
        
    void processTick();
    void processFoodEvent(unsigned foodID, unsigned howmuch);
    void processExerciseEvent(unsigned exerciseID, unsigned duration);
    void readFoodFile(const char* file);
    void readExerciseFile(const char* file);
    void readParams(const char* file);
    void stomachEmpty();
    bool isExercising();
    double currentEnergyExpenditure();
    double getGlucoseNeedsOutsideMuscles();

    double totalGlycolysisSoFar;
    double totalGNGSoFar;
    double totalOxidationSoFar;
    double totalExcretionSoFar;
    double totalLiverGlycogenStorageSoFar;
    double totalLiverGlycogenBreakdownSoFar;
    double totalMusclesGlycogenStorageSoFar;
    double totalMusclesGlycogenBreakdownSoFar;
    double totalGlucoseFromIntestineSoFar;
    double totalEndogeneousGlucoseReleaseSoFar;
    double totalGlucoseReleaseSoFar;
    void resetTotals(bool print);

    double glycolysis(double min, double max);

    double insulinImpactOnGlycolysis();
    double insulinImpactOnGNG();
    double insulinImpactOnGlycogenSynthesisInLiver();
    double insulinImpactOnGlycogenBreakdownInLiver();

	double tempGNG;
	double tempGlycolysis;
	double tempOxidation;
	double tempExcretion;
	double tempGlycogenStorage;
	double tempGlycogenBreakdown;

	double baseBGL; 
	double peakBGL;

 	double dailyCarbs;
    double intensityPeakGlucoseProd_; // exercise intensity in %VO2Maxat which peak GNG, glycogen breakdown takes place
private:
    double gngImpact_;
    double liverGlycogenBreakdownImpact_;

	double insulinImpactOnGlycolysis_Mean;
	double insulinImpactOnGNG_Mean;
	double insulinImpactGlycogenBreakdownInLiver_Mean;
	double insulinImpactGlycogenSynthesisInLiver_Mean;
	double insulinImpactOnGlycolysis_StdDev;
	double insulinImpactOnGNG_StdDev;
	double insulinImpactGlycogenBreakdownInLiver_StdDev;
	double insulinImpactGlycogenSynthesisInLiver_StdDev;

};


#endif
