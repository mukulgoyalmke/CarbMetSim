# CarbMetSim
A discrete event simulator for tracking blood glucose level based on carbodydrate metabolism in human body.

Authors:
--------------
Mukul Goyal (mukul@uwm.edu)

Buket Aydas (baydas@uwm.edu)

Husam Ghazaleh (ghazaleh@uwm.edu)

Tanawat Khunlerkit (tanawat@uwm.edu)


Usage:
----------------
The simulator is implemented in C++. A simple makefile has been provided.

Run the simulator in the following manner:

carbmetsim foodsfile exercisefile paramsfile eventsfile

"foodsfile":
--------------
The "foodsfile" contains the description of different food items. Each line in this file describes a different food using
the following format:

id food-description serving-size-in-grams RAG-per-serving SAG-per-serving Protein-per-serving Fat-per-serving

The "id" is an integer id. The food item is referred to in "eventsfile" using this "id".
The "food-description" is a text string (containing no space) that describes the food item.
The "serving-size-in-grams" specifies the size in grams of one serving of this food item.
The "RAG-per-serving" specifies the amount in grams of "Rapidly Available Glucose" (carbs that will appear in the bloodstream 
as glucose within 20 minutes of being eaten) in one serving of the food item.
The "SAG-per-serving" specifies the amount in grams of "Slowly Available Glucose" (carbs that will appear in the bloodstream 
as glucose within 120 minutes of being eaten) in one serving of the food item.
The "Protein-per-serving" specifies the amount in grams of protein in one serving of the food item.
The "Fat-per-serving" specifies the amount in grams of fat in one serving of the food item.

For example, the following line describes a "Breakfast" with serving size 100 grams, which consists of 25 grams of RAG, 30 
grams of SAG, 25 grams of protein and 20 grams of fat:

1 Breakfast 100 25 30 25 20
 
"exercisefile":
----------------
The "exercisefile" contains the description of different exercise types. Each line in this file describes a different exercise
type using the following format:

id exercise-description intensity-in-METs

The "id" is an integer id. The exercise type is referred to in "eventsfile" using this "id".
The "exercise-description" is a text string (containing no space) that describes the exercise type.
The "intensity-in-METs" specifies the intensity of the exercise type in units of METs.

For example, the following line describes a "SlowWalk" activity with intensity 3 METs:

1 SlowWalk 3

"paramsfile":
-------------------
The "paramsfile" describes the values of various parameters affecting the operation of different organs. The "paramsfile" can 
be used to modify the values of those parameters for which the default values are not appropriate. For example, the following 
lines in the "paramsfile" are setting the values of a few simulation parameters in "HumanBody" class:

ALL HUMAN_BODY insulinResistance_ 0

ALL HUMAN_BODY insulinPeakLevel_ 1.0

ALL HUMAN_BODY bodyWeight_ 65

"eventsfile":
-----------------
The "eventsfile" specifies the events that the simulator will simulate. Each line in this file specifies a different event in 
the following format:

time-stamp event-type event-subtype howmuch

Here, "time-stamp" indicates the time when the event will be fired and is specified in the following format "day:hour:minute".

The simulator currently supports the following "event-type"s:

0: Food Event

1: Exercise Event

2: Halt Event

If the "event-type" is 2 (indicating the Halt Event), the simulation terminates at the event firing time.

The "event-subtype" is same as the "id" of a food/exercise type in "foodsfile" or "exercisefile".

For a food event, "howmuch" indicates the amount in grams of the food eaten. For an exercise event, "howmuch" indicates the duration in minutes of the exercise activity.

Here is a sample "eventsfile":

0:8:0 0 1 100

0:13:0 0 2 135

0:16:0 0 3 60

0:18:0 1 1 30

0:20:0 0 4 135

1:6:0 2 0 0
