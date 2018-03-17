#include <iostream>
#include <math.h>
#include "PortalVein.h"
#include "Blood.h"
#include "HumanBody.h"

PortalVein::PortalVein(HumanBody* body_)
{
    body = body_;
    glucose = 0;
    branchedAA = 0;
    unbranchedAA = 0;
    fluidVolume_ = 5; // dl
}

void PortalVein::processTick()
{
    double bgl = body->blood->getBGL();
    double glucoseFromBlood = bgl*fluidVolume_;
    body->blood->removeGlucose(glucoseFromBlood);
    glucose += glucoseFromBlood;
    
    //SimCtl::time_stamp();
    //cout << " PortalVein:: " << glucose << " " << glucose/fluidVolume_ << " " << branchedAA << " " <<unbranchedAA << endl;
}

void PortalVein::releaseAllGlucose()
{
    body->blood->addGlucose(glucose);
    glucose = 0;
}

void PortalVein::removeGlucose(double g)
{
    glucose -= g;
    if( glucose < 0 )
    {
        cout << "PortalVein glucose went negative\n";
        exit(-1);
    }
}

double PortalVein::getConcentration()
{
    double gl = glucose/fluidVolume_;
    
    //SimCtl::time_stamp();
    //cout << "GL in Portal Vein: " << gl << endl;
    
    return gl;
}

void PortalVein::setParams()
{
    for( ParamSet::iterator itr = body->metabolicParameters[body->bodyState][PORTAL_VEIN].begin();
        itr != body->metabolicParameters[body->bodyState][PORTAL_VEIN].end(); itr++)
    {
        if(itr->first.compare("fluidVolume_") == 0)
        {
            fluidVolume_ = itr->second;
        }
    }
}

void PortalVein::addAminoAcids(double aa)
{
    branchedAA += 0.15*aa;
    unbranchedAA += 0.85*aa;
    //SimCtl::time_stamp();
    //cout << " PortalVein: bAA " << branchedAA << ", uAA " << unbranchedAA << endl;
}

void PortalVein::releaseAminoAcids()
{
    // 93% unbranched amino acids consumed by liver to make alanine
    body->blood->alanine += 0.93*unbranchedAA;
    body->blood->unbranchedAminoAcids += 0.07*unbranchedAA;
    unbranchedAA = 0;
    body->blood->branchedAminoAcids += branchedAA;
    branchedAA = 0;
    // who consumes these amino acids from blood other than liver?
    // brain consumes branched amino acids
}

