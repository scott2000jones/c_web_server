CXX= gcc
RM= rm -f

CFLAGS= -O3 -Wall -Wextra
LIBS= -pthread 

all: main
 
main:
	$(CXX) $(CFLAGS) $(LIBS) main.c -o main

clean:
	$(RM) a.out main