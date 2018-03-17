#ifndef __HumanBody__Intestine__
#define __HumanBody__Intestine__

#include <list>
#include "HumanBody.h"

struct Chyme // undigested carbs chyme
{
double origRAG;
double origSAG;
double RAG;
double SAG;
unsigned ts; // time (in ticks) when the chyme entered the intestine
};

class Intestine
{
    friend class HumanBody;

    list<Chyme> chyme;
    double protein; // mg

    double glucoseInLumen; // in milligrams
    double glucoseInEnterocytes; // in milligrams
    double fluidVolumeInEnterocytes_; // in deciliters
    double fluidVolumeInLumen_; // in deciliters
    //Michaelis Menten parameters for glucose transport
    double Glut2Km_In_;
    double Glut2VMAX_In_; //mg
    double Glut2Km_Out_;
    double Glut2VMAX_Out_; //mg
    //active transport rate
    double sglt1Rate_;
    double peakGlucoseConcentrationInLumen;
    
    //glycolysis
    double glycolysisMin_;
    double glycolysisMax_;
    
    //Amino Acid Absorption
    double aminoAcidsAbsorptionRate_;
    double glutamineOxidationRate_;
    double glutamineToAlanineFraction_;

    // Carb digestion parameters
    // support only normal distribution for RAG/SAG digestion so far.
    double RAG_Mean_;
    double RAG_StdDev_;
    double SAG_Mean_;
    double SAG_StdDev_;
    
    HumanBody* body;
    void absorbGlucose();
    void absorbAminoAcids();

    double glycolysisPerTick;
    double toPortalVeinPerTick;
public:
    Intestine(HumanBody* body_);
    void setParams();
    void processTick();
    void addChyme(double rag, double sag, double proteinInChyme, double fat);
};

#endif 
