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
    double insulinResistance_;
    double insulinPeakLevel_;
    double bodyWeight;
    double fatFraction_;
    unsigned currExercise;
    double currEnergyExpenditure; // current energy expenditure in kcal/minute per kg of body weight
    unsigned exerciseOverAt; // when does the current exercise event get over
    
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
};


#endif
