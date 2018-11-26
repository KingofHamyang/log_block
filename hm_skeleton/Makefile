#---------------------------------------------------------
#
# NAND Simulator
#
# v4 by Min-Woo Ahn, Dong-Hyun Kim (September 27, 2018)
# v3 by Dong-Yun Lee (April 20, 2017)
# v2 by Jin-Soo Kim (July 30, 2005)
#---------------------------------------------------------

PREFIX		=
CC			= $(PREFIX)gcc
CXX			= $(PREFIX)g++
WORKLOAD	= 	-DMULTI_HOT_COLD
				#-DHOT_COLD 
				# if -DHOT_COLD is not set, WORKLOAD will be random

FTL			= 	 
			# Log block FTL, by default

CFLAGS		= -g -O2 -Wall $(WORKLOAD) $(FTL)
LIBS		= 
RM			= rm
TAR			= tar

TARGET		= hm_sim
CSRCS		= hm_sim.c nand.c hm.c
CXXSRCS		= 
HEADERS		= nand.h hm.h
OBJS		= $(CSRCS:.c=.o) $(CXXSRCS:.cpp=.o)

all: $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LIBS) 

.c.o: $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.cpp.o: $(HEADERS)
	$(CXX) $(CFLAGS) -c $< -o $@

tar:
	$(RM) -f $(TARGET).tar.gz
	$(TAR) cvzf $(TARGET).tar.gz $(CSRCS) $(HEADERS) Makefile
	ls -l $(TARGET).tar.gz

clean:
	$(RM) -f $(TARGET) $(TARGET).tar.gz $(OBJS) *~
