#ifndef SIM_H
#define SIM_H

#include <string>
#include <chrono>
#include <random>

using namespace std;

#include "priq.h"

const int TICKS_PER_DAY = 24*60; // Simulated time granularity
const int TICKS_PER_HOUR = 60; // Simulated time granularity


enum EventType {FOOD = 0, EXERCISE, HALT, METFORMIN, INSULIN_SHORT, INSULIN_LONG};

class Event : public PriQElt {
public:
    inline Event(unsigned fireTime , EventType the_type);
    unsigned fireTime_;
    EventType eventType_;
};

//Constructor for Event
Event::Event(unsigned fireTime, EventType the_type) : PriQElt()
{
    fireTime_ = fireTime;
    eventType_ = the_type;
    cost0 = fireTime;
    cost1 = 0;
}

class FoodEvent : public Event {
public:
    inline FoodEvent(unsigned fireTime, unsigned quantity, unsigned foodID ): Event(fireTime, FOOD)
    {
        quantity_ = quantity;
        foodID_ = foodID;
    }
    unsigned quantity_; // in grams
    unsigned foodID_;
};

class ExerciseEvent : public Event {
public:
    inline ExerciseEvent(unsigned fireTime, unsigned duration, unsigned exerciseID ): Event(fireTime, EXERCISE)
    {
        duration_ = duration;
        exerciseID_ = exerciseID;
    }
    unsigned duration_;
    unsigned exerciseID_;
};

class HaltEvent : public Event {
public:
    inline HaltEvent(unsigned fireTime): Event(fireTime, HALT)
    {
    }
};


/* The global class implementing
 * the simulation controller.
 */

class SimCtl {
public:
    static unsigned 	ticks;
    static std::default_random_engine & myEngine();
    
    PriQ  eventQ;
    SimCtl();
    inline unsigned elapsed_days();
    inline unsigned elapsed_hours();
    inline unsigned elapsed_minutes();
    static void time_stamp();
    
    void readEvents(string file);
    void addEvent(unsigned fireTime, unsigned type, unsigned subtype, unsigned howmuch);
    int fire_event();
    void run_simulation();
    
    friend class HumanBody;
    friend int main(int argc, char *argv[]);
};


inline unsigned SimCtl::elapsed_days() {
    return(ticks/TICKS_PER_DAY);
}

inline unsigned SimCtl::elapsed_hours() {
    int x = ticks % TICKS_PER_DAY;
    return(x/TICKS_PER_HOUR);
}

inline unsigned SimCtl::elapsed_minutes() {
    int x = ticks % TICKS_PER_DAY;
    return (x % TICKS_PER_HOUR);
}

#endif

