#
# 'make'        build executable file 'carbmetsim'
# 'make clean'  removes all .o and executable files
#

#  -Wall turns on most, but not all, compiler warnings
#
# for C define  CC = gcc
CC = g++
CFLAGS  = -std=c++11 -Wall

#SRCS = priq.C SimCtl.cpp HumanBody.cpp Stomach.cpp Intestine.cpp PortalVein.cpp Liver.cpp Kidneys.cpp Muscles.cpp AdiposeTissue.cpp Blood.cpp Brain.cpp Heart.cpp

OBJS = priq.o SimCtl.o HumanBody.o Stomach.o Intestine.o PortalVein.o Liver.o Kidneys.o Muscles.o  AdiposeTissue.o Blood.o Brain.o Heart.o 

# define the executable file 
MAIN = carbmetsim

# typing 'make' will invoke the first target entry in the file 
# (in this case the default target entry)
# you can name this target entry anything, but "default" or "all"
# are the most commonly used names by convention
#
all:	$(MAIN)
	@echo  carbmetsim has been compiled

$(MAIN): $(OBJS) 
	$(CC) $(CFLAGS) -o $(MAIN) $(OBJS)

# this is a suffix replacement rule for building .o's from .C's
# it uses automatic variables $<: the name of the prerequisite of
# the rule(a .C file) and $@: the name of the target of the rule (a .o file) 
# (see the gnu make manual section about automatic variables)
.C.o:
	$(CC) $(CFLAGS) -c $<  -o $@

.cpp.o:
	$(CC) $(CFLAGS) -c $<  -o $@

# To start over from scratch, type 'make clean'.  This
# removes the executable file, as well as old .o object
# files and *~ backup files:
#
clean:
	$(RM) *.o *~ $(MAIN)

#depend: $(SRCS)
#	makedepend $^

## DO NOT DELETE THIS LINE -- make depend needs it
