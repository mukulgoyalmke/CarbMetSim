#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string>
#include <string.h>
#include <iostream>
#include <fstream>
#include "SimCtl.h"
#include "HumanBody.h"
#include <iomanip>
#include <fcntl.h>
#include <sys/stat.h>


using namespace std;

// Global variables

SimCtl* sim;
HumanBody	*body;
unsigned SimCtl::ticks=0;

void SimCtl::time_stamp()
{
    cout << sim->elapsed_days() << ":" << sim->elapsed_hours() << ":" <<  sim->elapsed_minutes() 
	<< " " << ticks << " ";
}

bool SimCtl::dayOver()
{
        if( ((ticks % TICKS_PER_DAY) == 0) && (ticks > 0) )
                return true;
        else
                return false;
}

void SimCtl::run_simulation()
{
    // Always in this loop
    while(true) {
        int val;
        
        while( (val = fire_event()) == 1 );
        
        // At this point:
        // no more event to fire;
        
        body->processTick();
        
        ticks++;
        
        //cout << elapsed_days() << ":" << elapsed_hours() << ":" << elapsed_minutes() << endl;
    }
}

int SimCtl::fire_event()
{
    Event *event_ = (Event *)eventQ.priq_gethead();
    
    if( !event_ )
    {
        cout << "No event left" << endl;
        exit(-1);
    }
    
    if (event_->fireTime_ > ticks)
        return -1;
    
    // Fire the event
    
    //cout << "ticks =" << ticks << ": " << elapsed_days() << "::" << elapsed_hours() << "::" << elapsed_minutes() << endl;
    //cout << "event_->fireTime_ : " << event_->fireTime_ << endl;
    
    EventType event_type = event_->eventType_;
    
    switch(event_type) {
        case FOOD:
            body->processFoodEvent( ((FoodEvent*)event_)->foodID_, ((FoodEvent*)event_)->quantity_);
            break;
        case EXERCISE:
            body->processExerciseEvent(((ExerciseEvent*)event_)->exerciseID_, ((ExerciseEvent*)event_)->duration_);
            break;
        case HALT:
	    SimCtl::time_stamp();
    	    cout << " weight " << body->bodyWeight << endl;
            exit(0);
        default:
            break;
    } // end switch case
   	
    event_ = (Event *)eventQ.priq_rmhead();
   	delete event_;
    return 1;
}

void SimCtl::addEvent(unsigned fireTime, unsigned type, unsigned subtype, unsigned howmuch)
{
    switch (type)
    {
        {
        case 0:
            FoodEvent* e = new FoodEvent(fireTime, howmuch, subtype);
            eventQ.priq_add(e);
            break;
        }
        {
        case 1:
            ExerciseEvent* f = new ExerciseEvent(fireTime, howmuch, subtype);
            eventQ.priq_add(f);
            break;
        }
        {
        case 2:
            HaltEvent* g = new HaltEvent(fireTime);
            eventQ.priq_add(g);
            break;
        }
        {
        default:
            break;
        }
    }
}

void  SimCtl::readEvents(string file)
{

    //ifstream cfg(file);
    ifstream ifl;
    ifl.open(file);
    string line;
    char* str = NULL;
    char* ts = NULL;
    
    if( ifl.is_open() )
    {
    
        while( getline(ifl,line) )
        {
            str = new char[line.length() + 1];
            strcpy(str, line.c_str());
            
		// get the time stamp
            char* tok = strtok(str, " ");
        
            
            //First token is the time stamp
            ts = new char[line.length() + 1];
            strcpy(ts, tok);

            tok = strtok(NULL, " ");
            unsigned type = (unsigned)atoi(tok);
            tok = strtok(NULL, " ");
            unsigned subtype = (unsigned)atoi(tok);
            tok = strtok(NULL, " ");
            unsigned howmuch = (unsigned)atoi(tok);
            
            tok = strtok(ts, ":");
            unsigned day = (unsigned)atoi(tok); // day
            tok = strtok(NULL, ":");
            unsigned hour = (unsigned)atoi(tok);
            tok = strtok(NULL, ":");
            unsigned minutes = (unsigned)atoi(tok);
            unsigned fireTime = day* TICKS_PER_DAY + hour* TICKS_PER_HOUR + minutes;

            //cout<< day<<":"<<hour<<":"<<minutes<< " " << type << " " << subtype << " " << howmuch << endl;
            
            
            addEvent(fireTime, type, subtype, howmuch);
            
            delete [] str;
            delete [] ts;
        }
        ifl.close();
    }
    else
    {
        cout << "Error opening " << file << endl;
        exit(1);
    }
}

SimCtl::SimCtl(string seed_string)
:generator(1)
{
    ticks = 0;

    std::seed_seq seed (seed_string.begin(),seed_string.end());
    generator.seed (seed);
}


/* Controlling process for the simulator.
 */

int main(int argc, char *argv[])
{
    cout << std::fixed;
    cout << std::setprecision(3);
    
    if (argc != 7) {
        cout << "Syntax: carbmetsim foodsfile exercisefile metabolicratesfile eventsfile seedstring outputfile\n";
        exit(1);
    }

    // redirect stdout
    int fd = open(argv[6], O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd == -1) {
       cout << "Failed to open " << argv[6] << endl;
       return 1;
    }
    if (dup2(fd, STDOUT_FILENO) == -1) {
      cout << "Failed to redirect standard output" << endl;
      return 1;
   }
   if (close(fd) == -1) {
      cout << "Failed to close the file " << argv[6] << endl;
      return 1;
   }

    // Create simulation controller
    sim= new SimCtl(argv[5]);
    body = new HumanBody();
    body->readFoodFile(argv[1]);
    body->readExerciseFile(argv[2]);
    body->readParams(argv[3]); // read the organs parameters for various states.
    body->setParams(); // set the organ parameters corresponding to the body's initial state.
    sim->readEvents(argv[4]);
    //srand(atoi(argv[5]));
    sim->run_simulation();
    return 0;
}

