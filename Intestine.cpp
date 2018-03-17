#include <iostream>
#include <math.h>
#include "Intestine.h"
#include "AdiposeTissue.h"
#include "Blood.h"
#include "HumanBody.h"
#include "PortalVein.h"

void Intestine::addChyme(double rag, double sag, double proteinInChyme, double fat)
{
	Chyme c;
	c.RAG = rag;
	c.SAG = sag;
	c.origRAG = rag;
	c.origSAG = sag;
	c.ts = SimCtl::ticks;
	chyme.push_back(c);

	protein += proteinInChyme;

    // very simple processing of fat for now
    body->adiposeTissue->addFat(fat);
}

void Intestine::processTick()
{
	// digest some chyme
	for(list<Chyme>::iterator itr = chyme.begin(); itr != chyme.end(); itr++)
	{
    		double RAGConsumed = 0;
    
    		double t = SimCtl::ticks - (*itr).ts;

		//we assume that RAG appears in blood as per a normal distributon with a user specified mean/stddev
    
    		if( t == 0 )
        		RAGConsumed = (itr->origRAG)*0.5*(1 + erf((t - RAG_Mean_)/(RAG_StdDev_*sqrt(2))));
    		else
        		RAGConsumed = (itr->origRAG)*0.5*( erf((t-RAG_Mean_)/(RAG_StdDev_*sqrt(2))) -erf((t-1-RAG_Mean_)/(RAG_StdDev_*sqrt(2))) );
    
    		if( itr->RAG < RAGConsumed )
        		RAGConsumed = itr->RAG;
    
		if( itr->RAG < 0.01*(itr->origRAG) )
			RAGConsumed = itr->RAG;

    		itr->RAG -= RAGConsumed;
    		glucoseInLumen += RAGConsumed;

    		// digest some SAG now
    
    		double SAGConsumed = 0;
    
        	// we assume that SAG appears in blood as per a normal distributon with a user specified mean/stdev
        
        	if( t == 0 )
            		SAGConsumed = (itr->origSAG)*0.5*(1 + erf((t - SAG_Mean_)/(SAG_StdDev_*sqrt(2))));
        	else
            		SAGConsumed = (itr->origSAG)*0.5*( erf((t-SAG_Mean_)/(SAG_StdDev_*sqrt(2))) -erf((t-1-SAG_Mean_)/(SAG_StdDev_*sqrt(2))) );
                
        	if( itr->SAG < SAGConsumed )
            		SAGConsumed = itr->SAG;
        
		if( itr->SAG < 0.01*(itr->origSAG) )
			SAGConsumed = itr->SAG;

        	itr->SAG -= SAGConsumed;
        	glucoseInLumen += SAGConsumed;

    		//SimCtl::time_stamp();
    		//cout << " Chyme:: RAG " << itr->RAG << " SAG " << itr->SAG << " origRAG " << itr->origRAG 
		//<< " origSAG " << itr->origSAG << " glucoseInLumen " << glucoseInLumen << " RAGConsumed " 
		//<< RAGConsumed << " SAGConsumed " << SAGConsumed << endl;

    		if( itr->RAG == 0 && itr->SAG == 0 )
    			itr = chyme.erase(itr);
	}

    	// some of the glucose is absorbed by the enterocytes (some of which moves to the portal vein)
    	 absorbGlucose();
    	 absorbAminoAcids();

	SimCtl::time_stamp();
        cout << " Intestine:: Glycolysis " << glycolysisPerTick << endl;
	SimCtl::time_stamp();
        cout << " Intestine:: ToPortalVein " << toPortalVeinPerTick << endl;
}

void Intestine::setParams()
{
    for( ParamSet::iterator itr = body->metabolicParameters[body->bodyState][INTESTINE].begin();
        itr != body->metabolicParameters[body->bodyState][INTESTINE].end(); itr++)
    {
        if(itr->first.compare("aminoAcidAbsorptionRate_") == 0)
        {
            aminoAcidsAbsorptionRate_ = itr->second;
        }
        if(itr->first.compare("glutamineOxidationRate_") == 0)
        {
            glutamineOxidationRate_ = itr->second;
        }
        if(itr->first.compare("glutamineToAlanineFraction_") == 0)
        {
            glutamineToAlanineFraction_ = itr->second;
        }
        if(itr->first.compare("Glut2VMAX_In_") == 0)
        {
            Glut2VMAX_In_ = itr->second;
        }
        if(itr->first.compare("Glut2Km_In_") == 0)
        {
            Glut2Km_In_ = itr->second;
        }
        if(itr->first.compare("Glut2VMAX_Out_") == 0)
        {
            Glut2VMAX_Out_ = itr->second;
        }
        if(itr->first.compare("Glut2Km_Out_") == 0)
        {
            Glut2Km_Out_ = itr->second;
        }
        if(itr->first.compare("sglt1Rate_") == 0)
        {
            sglt1Rate_ = itr->second;
        }
        if(itr->first.compare("fluidVolumeInLumen_") == 0)
        {
            fluidVolumeInLumen_ = itr->second;
        }
        if(itr->first.compare("fluidVolumeInEnterocytes_") == 0)
        {
            fluidVolumeInEnterocytes_ = itr->second;
        }
        if(itr->first.compare("glycolysisMin_") == 0)
        {
            glycolysisMin_ = itr->second;
        }
        if(itr->first.compare("glycolysisMax_") == 0)
        {
            glycolysisMax_ = itr->second;
        }
        if(itr->first.compare("RAG_Mean_") == 0)
        {
                RAG_Mean_ = itr->second;
        }
        if(itr->first.compare("RAG_StdDev_") == 0)
        {
                RAG_StdDev_ = itr->second;
        }
        if(itr->first.compare("SAG_Mean_") == 0)
        {
                SAG_Mean_ = itr->second;
        }
        if(itr->first.compare("SAG_StdDev_") == 0)
        {
                SAG_StdDev_ = itr->second;
        }        
    }
}

Intestine::Intestine(HumanBody* body_)
{
    body = body_;

    RAG_Mean_ = 5;
    RAG_StdDev_ = 5;
    SAG_Mean_ = 60;
    SAG_StdDev_ = 20;
    
    protein = 0; // mg
    glucoseInLumen = 0; // in milligrams
    glucoseInEnterocytes = 0; // in milligrams
    
    // Carb digestion parameters
    // support only normal distribution for RAG/SAG digestion so far.
    fluidVolumeInEnterocytes_ = 3; //dl
    fluidVolumeInLumen_ = 4; //dl
    
    //Michaelis Menten parameters for glucose transport
    Glut2Km_In_ = 20*180.1559/10.0; // mg/deciliter equal to 20 mmol/l (Frayn Table 2.2.1)
    Glut2VMAX_In_ = 700; //mg
    Glut2Km_Out_ = 20*180.1559/10.0; // mg/deciliter equal to 20 mmol/l (Frayn Table 2.2.1)
    Glut2VMAX_Out_ = 700; //mg
    //active transport rate
    sglt1Rate_ = 30; //mg per minute
    
    peakGlucoseConcentrationInLumen = 200*180.1559/10.0; // mg/dl equivalent of 200mmol/l
    
    aminoAcidsAbsorptionRate_ = 1; //mg per minute
    glutamineOxidationRate_ = 1; // mg per minute
    glutamineToAlanineFraction_ = 0.5;
    
    //Gerich: insulin dependent: 1 to 5 micromol per kg per minute
    glycolysisMin_ = 0.1801559;
    glycolysisMax_ = 5*glycolysisMin_;
}

void Intestine::absorbGlucose()
{
    double x; // to hold the random samples
    double activeAbsorption = 0;
    double passiveAbsorption = 0;
    
    double glLumen = 0;
    double glEnterocytes = 0;
    double glPortalVein = 0;
    
    static std::poisson_distribution<int> basalAbsorption__ (1000.0*sglt1Rate_);
    static std::poisson_distribution<int> Glut2VMAX_In__ (1000.0*Glut2VMAX_In_);
    static std::poisson_distribution<int> Glut2VMAX_Out__ (1000.0*Glut2VMAX_Out_);
    static std::poisson_distribution<int> glycolysisMin__ (1000.0*glycolysisMin_); 

    // first, absorb some glucose from intestinal lumen
    
    if( glucoseInLumen > 0 )
    {
        if ( fluidVolumeInLumen_ <= 0 )
        {
            cout << "Intestine::absorbGlucose" << endl;
            exit(-1);
        }
    
        // Active transport first
        activeAbsorption = (double)(basalAbsorption__(SimCtl::myEngine()))/1000.0;
        
        if( activeAbsorption >= glucoseInLumen )
        {
            activeAbsorption = glucoseInLumen;
            glucoseInEnterocytes += activeAbsorption;
	    glucoseInLumen = 0;
        }
        else
        {
            glucoseInEnterocytes += activeAbsorption;
	    glucoseInLumen -= activeAbsorption;
    
            //passive transport via GLUT2s now
            glLumen = glucoseInLumen/fluidVolumeInLumen_;
            glEnterocytes = glucoseInEnterocytes/fluidVolumeInEnterocytes_;
            double diff = glLumen - glEnterocytes;
            
            if( diff > 0 )
            {
                // glucose concentration in lumen decides the number of GLUT2s available for transport.
                // so, Vmax depends on glucose concentration in lumen
                x = (double)(Glut2VMAX_In__(SimCtl::myEngine()))/1000.0;
                double effectiveVmax = x*glLumen/peakGlucoseConcentrationInLumen;
    
                if (effectiveVmax > Glut2VMAX_In_)
                    effectiveVmax = Glut2VMAX_In_;
                
                passiveAbsorption = effectiveVmax*diff/(diff + Glut2Km_In_);
    
                if ( passiveAbsorption > glucoseInLumen )
                    passiveAbsorption = glucoseInLumen;
                
                glucoseInEnterocytes += passiveAbsorption;
                glucoseInLumen -= passiveAbsorption;
            }
        }
    }
    
    //release some glucose to portal vein via Glut2s
    glEnterocytes = glucoseInEnterocytes/fluidVolumeInEnterocytes_;
    glPortalVein = body->portalVein->getConcentration();
    
    toPortalVeinPerTick = 0;
    
    double diff = glEnterocytes - glPortalVein;
    
    if(diff > 0 )
    {
        x = (double)(Glut2VMAX_Out__(SimCtl::myEngine()))/1000.0;
        toPortalVeinPerTick = x*diff/(diff + Glut2Km_Out_);
        
        if( toPortalVeinPerTick > glucoseInEnterocytes )
            toPortalVeinPerTick = glucoseInEnterocytes;
        
        glucoseInEnterocytes -= toPortalVeinPerTick;
        body->portalVein->addGlucose(toPortalVeinPerTick);
    }
    
    // Modeling the glucose consumption by enterocytes: glycolysis to lactate.
    
    //Glycolysis. Depends on insulin level. Consumed glucose becomes lactate (Ref: Gerich).
    
    double scale = (1.0 - body->insulinResistance_)*(body->blood->insulinLevel);
    
    x = (double)(glycolysisMin__(SimCtl::myEngine()));
    x *= body->bodyWeight/1000.0;
    if( x > glycolysisMax_*(body->bodyWeight))
        x = glycolysisMax_*(body->bodyWeight);
    
    glycolysisPerTick = x + scale* ( (glycolysisMax_*(body->bodyWeight)) - x);
    
    if( glycolysisPerTick > glucoseInEnterocytes)
    {
	body->blood->removeGlucose(glycolysisPerTick - glucoseInEnterocytes);
    	glucoseInEnterocytes = 0;
    }
    else
    {
    	glucoseInEnterocytes -= glycolysisPerTick;
    }

    body->blood->lactate += glycolysisPerTick;
    
    // log all the concentrations (in mmol/l)
    // peak concentrations should be 200mmol/l (lumen), 100mmol/l(enterocytes), 10mmol/l(portal vein)
    
    glLumen = (10.0/180.1559)*glucoseInLumen/fluidVolumeInLumen_; // in mmol/l
    glEnterocytes = (10.0/180.1559)*glucoseInEnterocytes/fluidVolumeInEnterocytes_;
    x = body->portalVein->getConcentration();
    glPortalVein = (10.0/180.1559)*x;

    SimCtl::time_stamp();
    cout << " Intestine:: glLumen: " << glLumen << " glEntero " << glEnterocytes << " glPortal " << glPortalVein << 
	", " << x << " activeAbsorption " << activeAbsorption << " passiveAbsorption " << passiveAbsorption << endl;
}

//The BCAAs, leucine, isoleucine, and valine, represent 3 of the 20 amino acids that are used in the formation of proteins.
// Thus, on average, the BCAA content of food proteins is about 15% of the total amino acid content.i
// "Interrelationship between Physical Activity and Branched-Chain Amino Acids"

//The average content of glutamine in protein is about %3.9. "The Chemistry of Food" By Jan Velisek
//Do we consider the dietary glutamine? I did not consider in my code but I can add if we need it.

//Looks like cooking destroys dietary glutamine. So, no need to consider diet as source of glutamine.
//-Mukul

void Intestine::absorbAminoAcids()
{
    static std::poisson_distribution<int> aminoAcidsAbsorptionRate__(1000.0*aminoAcidsAbsorptionRate_);
    static std::poisson_distribution<int> glutamineOxidationRate__(1000.0*glutamineOxidationRate_);
    
    double absorbedAA = (double)(aminoAcidsAbsorptionRate__(SimCtl::myEngine()))/1000.0;
    
    if( protein < absorbedAA )
    {
        absorbedAA = protein;
    }
    
    body->portalVein->addAminoAcids(absorbedAA);
    protein -= absorbedAA;
    
    //Glutamine is oxidized
    double g = (double)(glutamineOxidationRate__(SimCtl::myEngine()))/1000.0;
    if( body->blood->glutamine < g )
    {
            body->blood->alanine += glutamineToAlanineFraction_*(body->blood->glutamine);
            body->blood->glutamine = 0;
    }
    else
    {
        body->blood->glutamine -= g;
        body->blood->alanine += glutamineToAlanineFraction_*g;
    }
}

