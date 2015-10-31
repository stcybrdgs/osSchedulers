/****************************************************
*   course:       CS4328 
*   project:      Project 1 - Scheduler Simulator 
*   programmer:   Stacy Bridges
*   date:         10/29/2015  
*   description:  for description, see "main.cpp"
****************************************************/
#include "header.h"
#include <cmath>
#include <stdio.h>
#include <stdlib.h>               // rand()
#include <iostream>
using namespace std;

/////////////////////////////////////////////////////
// define structs
struct cpuNode{
   float simClock;                // simulation clock    
   bool cpuIsBusy;                // busy flag
	int preemptedFlag;				 // 0=unknown, 1=preempted, 
											 // 2=not preempted
   struct procListNode* pLink;    // the target process
};

struct readyQNode{
   struct procListNode* pLink;    // point to matching process in process list 
   struct readyQNode* rNext;      // point to next process in the ready queue
};

struct eventQNode{
   float time;          
   int type;                      // 1=arrival; 2=departure; 
											 // 3=allocation; 4=preemption   
   struct eventQNode* eNext;      // point to next event in the event queue
   struct procListNode* pLink;    // point to matching process in process list
};

struct procListNode{
   float arrivalTime;    
   float startTime;
	float lastStartTime;
   float finishTime;
   float serviceTime;
   float remainingTime;
   struct procListNode* pNext;    // point to next process in the list
};

/////////////////////////////////////////////////////
// delcare global variables
int schedulerType;               // argv[1] from console
int lambda;                      // argv[2] from console
float avgTs;                     // argv[3] from console
float quantum;                   // argv[4] from console
int batchSize;                   // argv[5] from console
float mu;                        // 1/avgTs
eventQNode* eHead;               // head of event queue
procListNode* pHead;             // head of oprocess list
readyQNode* rHead;               // head of ready queue
cpuNode* cpuHead;                // the cpu node (there's only one)

/////////////////////////////////////////////////////
// helper function
void insertIntoEventQ(eventQNode*);

/////////////////////////////////////////////////////
// define function implementations

// initialize global variables to values of args from console
void parseArgs(char *argv[]){
	schedulerType = atoi( argv[1] );
	lambda = atoi( argv[2] );
	avgTs = (float)atof( argv[3] );   
	quantum = (float)atof( argv[4] );
	batchSize = atoi( argv[5] );
}

// initialize all variable, states, and end conditions
void init(){
	
	// mu is used in genExp(float) to get service time  
   mu = (float)1.0/avgTs;     
   
	// create the cpu node
   cpuHead = new cpuNode; 
   cpuHead->simClock = 0.0;
   cpuHead->cpuIsBusy = 0;        // cpu flag: 0=false=idle, 1=true=busy 
   cpuHead->pLink = 0;
	cpuHead->preemptedFlag = 0;    // 0=unknown, 1=preempted, 
											 // 2=not preempted
	       
   // create process list node, point pHead to it, initialize member vars
   // this first node is being initialized as the first process for the sim
   pHead = new procListNode;
   pHead->arrivalTime = genExp((float)lambda);   
   pHead->startTime = 0.0;
   pHead->lastStartTime = 0.0;
   pHead->finishTime = 0.0;
   pHead->serviceTime = genExp(mu);  
   pHead->remainingTime = pHead->serviceTime;
   pHead->pNext = 0;
   
   // create event queue node, point pHead to it, initialize member vars
   eHead = new eventQNode;
   eHead->time = pHead->arrivalTime;
   eHead->type = 1;
   eHead->eNext = 0;
   eHead->pLink = pHead;   
}

void run_sim(){
   switch ( schedulerType ){
     case 1:
        cout << "The sim is running FCFS. . . " << endl;
        FCFS();
        break;
     case 2:
        cout << "The sim is running SRTF. . . " << endl;
        SRTF();
        break;
     case 3:   
        cout << "The sim is running HRRN. . . " << endl;
        HRRN();
        break;
     case 4:   
        cout << "The sim is running RR. . . " << endl;
        RR();
        break;        
     default:	
        cout << "Error in run_sim(). . . " << endl;
   }
}

void tester(){
	for( int i = 0; i < 5; i ++ ){
		scheduleArrival();
		handleArrival();
	}
	scheduleAllocation();
	handleAllocation();	
	scheduleDeparture();	
	handleDeparture();	
	printQueues();
}

void FCFS(){
	int arrivalCount = 0;
	int departureCount = 0;
	while( departureCount < batchSize ){ 		 
		// case 1: cpu is not busy -------------------
		if( cpuHead->cpuIsBusy == false ){
			if( arrivalCount < (batchSize * 1.10) ){
				scheduleArrival();
				arrivalCount++;
			}
			if( rHead != 0 ){
				scheduleAllocation();
			}
		}
		// case 2: cpu is busy -------------------
		else{
			scheduleDeparture();
		}

		// now, handle the next event in the eventQ
		if( eHead->type == 1 ){
			handleArrival();			
		}
		else if( eHead->type == 2 ){  
			handleDeparture();
			departureCount++;
		}
		else if( eHead->type == 3 ){
			handleAllocation();
		}
		else{
			cout << "Error in FCFS()." << endl;
		}
	} // end while
}

void SRTF(){
	int arrivalCount = 0;
	int departureCount = 0;

	while( departureCount < batchSize ){ 		  
		// schedule an arrival on every iteration up to threshold
		if( arrivalCount < (batchSize * 1.10) ){
			scheduleArrival();
			arrivalCount++;
		}
		// case 1: cpu is not busy -------------------
		if( cpuHead->cpuIsBusy == false ){
			if( rHead != 0 ){
				scheduleAllocation();	
			}
		}
		// case 2: cpu is busy -------------------
		else{
			if( cpuHead->preemptedFlag == 2 ){
				scheduleDeparture();
			}
		}
		// all: on every iteration, handle the next event in eventQ
		if( eHead == 0 ){
			scheduleArrival();				
		}
		if( eHead->type == 1 && cpuHead->cpuIsBusy == 1 ){
			// when cpu is busy, if a new arrival time is more than the
			// estimated cpu finish time, then set the preemptedFlag to 
			// indicate it's ok to schedule a departure on the next pass
			if( eHead->time > cpuEstFinishTime() ){
				cpuHead->preemptedFlag = 2;
			}
			// else, if new arrival time is less than est. finish time, then
			// check if the arrival preempts; if so, schedule a preempt event
			else if( isPreempted() == 1 ){
				schedulePreemption();		
				cpuHead->preemptedFlag = 1;
			}
		}
		// now handle all other types of events; for the arrivals, only handle 
		// the ones that occur when no preempting or departing is occuring
		if( eHead->type == 1 && cpuHead->preemptedFlag == 0 ){
			handleArrival();			// handle arrival
		}
		if( eHead->type == 2) {         
			handleDeparture();		// handle departure	
			departureCount++;
		}
		if( eHead->type == 3 ){        
			handleAllocation();		// handle allocation		
		}
		if( eHead->type == 4 ){         
			handlePreemption();		// handle preemption
		}
	} // end while
}

bool isPreempted(){
	cout << "isPreempted() " << endl;
	float cpuRT = cpuEstFinishTime() - eHead->time;
	float nuArrivalRT = eHead->pLink->remainingTime;

	if( cpuRT > nuArrivalRT ) return true;
	else return false;
}

float cpuEstFinishTime(){
	return ( cpuHead->pLink->lastStartTime + 
		      cpuHead->pLink->remainingTime );
}

void HRRN(){}
void RR(){}

// adds a process to process list and
// schedules an arrival event in eventQ
void scheduleArrival(){
	cout << "schedule arrival" << endl;

   // add a process to the process list 
   procListNode* pIt = pHead;
   while( pIt->pNext !=0 ){
      pIt = pIt->pNext;
   }
   pIt->pNext = new procListNode;
   pIt->pNext->arrivalTime = pIt->arrivalTime + genExp((float)lambda);
   pIt->pNext->startTime = 0.0;
   pIt->pNext->lastStartTime = 0.0;
   pIt->pNext->finishTime = 0.0;
   pIt->pNext->serviceTime = genExp(mu);
   pIt->pNext->remainingTime = pIt->pNext->serviceTime;
   pIt->pNext->pNext = 0;
  
   // create a corresponding arrival event 
   eventQNode* nuArrival = new eventQNode;
   nuArrival->time = pIt->pNext->arrivalTime;
   nuArrival->type = 1;
   nuArrival->pLink = pIt->pNext;
   nuArrival->eNext = 0;    

	// insert into eventQ in asc time order
	insertIntoEventQ(nuArrival);
}

// schedules an allocation event in eventQ
void scheduleAllocation(){
	cout << "schedule allocation" << endl;

	// create an allocation event 
   eventQNode* nuAllocation = new eventQNode;
	if( cpuHead->simClock < rHead->pLink->arrivalTime ){
		nuAllocation->time = rHead->pLink->arrivalTime;
	}
	else{
		nuAllocation->time = cpuHead->simClock;	
	}
   nuAllocation->type = 3;
   nuAllocation->pLink = rHead->pLink;
   nuAllocation->eNext = 0;    

	// insert into eventQ in asc time order
	insertIntoEventQ(nuAllocation);
}

// schedules a departure event in eventQ
void scheduleDeparture(){
	cout << "schedule departure" << endl;

	// create a new event node for the departure event     
   eventQNode* nuDeparture = new eventQNode;
   nuDeparture->time = ( cpuHead->simClock + cpuHead->pLink->remainingTime );
   nuDeparture->type = 2;
   nuDeparture->eNext = 0;
   nuDeparture->pLink = cpuHead->pLink;

	// insert into eventQ in asc time order
	insertIntoEventQ(nuDeparture);
}

// moves process from eventQ to readyQ
void handleArrival(){
	cout << "handle arrival" << endl;

	// create a new readyQ node based on proc in eHead
	readyQNode* nuReady = new readyQNode;
   nuReady->pLink = eHead->pLink;
	nuReady->rNext = 0;

	// push the new node into the readyQ 
	if( rHead == 0 ) rHead = nuReady;
	else{
		readyQNode* rIt = rHead;
		while( rIt->rNext != 0 ) {
			rIt = rIt->rNext;
		}
		rIt->rNext = nuReady;
	}

   // pop the arrival from the eventQ
	popEventQHead();
}

// moves process from readyQ to CPU
void handleAllocation(){
	cout << "handle allocation" << endl;

	cpuHead->pLink = rHead->pLink;			// point cpu to readyQ proc
	cpuHead->cpuIsBusy = true;					// update busy flag
	cpuHead->simClock = eHead->time;			// update sim clock
	cpuHead->pLink->lastStartTime =			// update last start time
		cpuHead->simClock;
	popReadyQHead();								// pop proc from readyQ
	popEventQHead();								// pop allocation event
	
	if( cpuHead->pLink->startTime == 0 ){	// update start time  
		cpuHead->pLink->startTime = cpuHead->simClock;
	}
}

// terminates a process and clears the cpu
void handleDeparture(){
	cout << "handle departure" << endl;

   // update cpu data
   cpuHead->pLink->finishTime = eHead->time;	
	cpuHead->pLink->remainingTime = 0.0;		
   cpuHead->pLink = 0;								
   cpuHead->simClock = eHead->time;	
   cpuHead->cpuIsBusy = false;
	cpuHead->preemptedFlag = 0;
		
	// pop the departure from the eventQ
	popEventQHead();
}

// schedules a preemption event in eventQ
void schedulePreemption(){
	cout << "schedule preemption" << endl;

   // create a new event node for the preemption event
   eventQNode* nuPreemption = new eventQNode;
   nuPreemption->time = eHead->pLink->arrivalTime;
   nuPreemption->type = 4;
   nuPreemption->eNext = 0;
   nuPreemption->pLink = eHead->pLink;

   // pop the pre-empting arrival from the eventQ
	popEventQHead();

	// insert into eventQ in asc time order
	insertIntoEventQ(nuPreemption);
}

// moves preempted proc out of cpu and preempting proc into cpu
void handlePreemption(){
	cout << "handle preemption" << endl;

	// update remaining time for process that is in the cpu
	float remainingTime = cpuHead->pLink->remainingTime;
	float timeRunning = eHead->time - cpuHead->pLink->lastStartTime;
	float nuRemainingTime = remainingTime - timeRunning;
	cpuHead->pLink->remainingTime = nuRemainingTime;

	// create a new readyQ node to hold the preempted process
	readyQNode* preEmptedProc = new readyQNode;
	preEmptedProc->pLink = cpuHead->pLink;
	preEmptedProc->rNext = 0;
	
	// append preEmpted to readyQ
	if( rHead == 0 ){
		rHead = preEmptedProc;
	}
	else if( rHead->rNext == 0 ){
		rHead->rNext = preEmptedProc;
	}
	else{
		readyQNode* rIt = rHead;
		while( rIt->rNext != 0 ){
			rIt = rIt->rNext;
		}
		rIt->rNext = preEmptedProc;
	}

	// put the preempting proc into the cpu
	cpuHead->simClock = eHead->time;			// update sim clock event time
	cpuHead->pLink = eHead->pLink;			// point cpu to eHead proc
	cpuHead->preemptedFlag = 0;
	cpuHead->pLink->lastStartTime =			// update last start time 
	cpuHead->simClock;
	if( cpuHead->pLink->startTime == 0 ){  // update start time
		cpuHead->pLink->startTime = cpuHead->simClock;
	}

	// pop the preemption from the eventQ
	popEventQHead();
}

/////////////////////////////////////////////////////
// helper functions

// returns a random number bewteen 0 and 1
float urand(){
	return( (float) rand() / RAND_MAX );
}

// returns a random number that follows an exp distribution
float genExp(float val){
	float u, x;
   x = 0;
   while( x == 0 ){
		u = urand();
		x = (-1/val)*log(u);    
	}
   return x;
}

void insertIntoEventQ( eventQNode* nuEvent){
	// put the new event in the readyQ, sorted by time
   if( eHead == 0 ) eHead = nuEvent;
   else if( eHead->time > nuEvent->time ){
      nuEvent->eNext = eHead;
      eHead = nuEvent;
   }
   else{
      eventQNode* eIt = eHead;
      while( eIt != 0 ){
         if( (eIt->time < nuEvent->time) && (eIt->eNext == 0) ){
            eIt->eNext = nuEvent; 
         }             
         else if( (eIt->time < nuEvent->time) && 
                  (eIt->eNext->time > nuEvent->time)){
            nuEvent->eNext = eIt->eNext;
            eIt->eNext = nuEvent;                                    
         }
         else{
            eIt = eIt->eNext;
         }
      }     
   } 
}

void popEventQHead(){
	eventQNode* tempPtr = eHead;
	eHead = eHead->eNext;
	delete tempPtr;
}

void popReadyQHead(){
	readyQNode* tempPtr = rHead;
	rHead = rHead->rNext;
	delete tempPtr;
}

/////////////////////////////////////////////////////
// metric computation functions
float getAvgTurnaroundTime(){
   float totTurnaroundTime = 0.0; 
   procListNode* pIt = pHead;
   while( pIt->finishTime != 0 ){
      // tally up the turnaround times
      totTurnaroundTime += ( pIt->finishTime - pIt->arrivalTime );
      pIt = pIt->pNext;  
   }
   return (totTurnaroundTime/batchSize);
}

float getTotalThroughput(){
   procListNode* pIt = pHead;
   float finTime = 0.0;
   while( pIt->finishTime != 0 ){
      // get the final timestamp
      if( pIt->pNext->finishTime == 0 )
         finTime = pIt->finishTime;
      pIt = pIt->pNext;    
   }
   return ( (float)batchSize / finTime );   
}

float getCpuUtil(){
   procListNode* pIt = pHead;
   float busyTime = 0.0;
   float finTime = 0.0;   
   while( pIt->finishTime != 0 ){
      busyTime += pIt->serviceTime;  
      if( pIt->pNext->finishTime == 0 )
         finTime = pIt->finishTime;      
      pIt = pIt->pNext; 
   }
   return ( busyTime / finTime );   
}

float getAvgNumProcInQ(){
   // identify the final second of processing (timeN)
	// as it would appear on a seconds-based timeline
   float timeNmin1 = 0.0;  
   procListNode* pIt = pHead;
   while( pIt->finishTime != 0 ){
      if( pIt->pNext->finishTime == 0 )
         timeNmin1 = pIt->finishTime;
      pIt = pIt->pNext;
   }
   int timeN = static_cast<int>(timeNmin1) + 1;
   
   // tally up the total processes in the ready queue
   // for each second of the seconds-based timeline
   pIt = pHead;
   int time = 0;;
   int numProcsInQ = 0;
   for( time = 0; time < timeN; time++ ){
      while( pIt->finishTime != 0 ){
         if( ( pIt->arrivalTime < time && pIt->startTime > time ) ||
             ( pIt->arrivalTime > time && pIt->arrivalTime < (time + 1) ) ){
            numProcsInQ ++;
         } 
         pIt = pIt->pNext;       
     }
     pIt = pHead;
   }
   
   return ( (float)numProcsInQ / timeN );
}

/////////////////////////////////////////////////////
// test functions
void printQueues(){
  // print process list data
   procListNode* pIt = pHead;
   cout << endl;
   cout << "process list: --------------------" << endl;
   cout << "arrivalTime\t\t serviceTime\t\t startTime\t\t lastStartTime\t\tfinishTime\t\t remainingTime\t\t" << endl;
   while( pIt != 0 ){
      cout << pIt->arrivalTime << "  \t\t"
           << pIt->serviceTime << "  \t\t";
           if( pIt->startTime == 0 ) cout << "________0\t\t"; else cout << pIt->startTime << "  \t\t";
           if( pIt->lastStartTime == 0 ) cout << "________0\t\t"; else cout << pIt->lastStartTime << "  \t\t";
           if( pIt->finishTime == 0 ) cout << "________0\t\t"; else cout << pIt->finishTime << "  \t\t";
           if( pIt->remainingTime == 0 ) cout << "________0\t\t"; else cout << pIt->remainingTime << "  \t\t";
           cout << endl;
      pIt = pIt->pNext;
   }
   cout << endl;
   
   // print event queue data
   eventQNode* eIt = eHead;
   cout << "event queue: --------------------" << endl;
   cout << "time           type         pLinkAT" << endl;   
   while( eIt != 0 ){
      cout << eIt->time << "\t";
		if( eIt->type == 1 ) cout << "arrival";
		else if ( eIt->type == 2 ) cout << "departure";
		else if ( eIt->type == 3 ) cout << "allocation";
		else cout << "preemption";
		cout << "\t\t" << eIt->pLink->arrivalTime 
           << cpuHead->simClock  << "  \t\t"			
			  << endl; 
      eIt = eIt->eNext;     
   }   
   cout << endl;   

   // print ready queue data
   if( rHead != 0 ){
      // print ready queue data
      readyQNode* rIt;
      rIt = rHead;
      cout << "ready queue: --------------------" << endl;
      cout << "pLinkAT\t pLinkRT\t" << endl;   
      
		while( rIt != 0 ){
         cout << rIt->pLink->arrivalTime << "  \t\t" 
				  << rIt->pLink->remainingTime 
              << cpuHead->simClock  << "  \t\t"				  
				  << endl; // LOOP ISSUE HERE XXXXXXXXXXXXXXXXXXX
         rIt = rIt->rNext;     
      }  
   } 
	else{
      cout << "ready queue: --------------------" << endl;
      cout << "No data: the ready queue is empty" << endl; 
	}
   cout << endl;

   // print cpu data
      cout << "cpu: --------------------" << endl;
      cout << "simClock\t cpuIsBusy\t procArrival\t procStart\t " << endl;   
		cout  << cpuHead->simClock << "\t\t"
				<< cpuHead->cpuIsBusy << "\t\t"
				<< endl;   
		if( cpuHead->pLink != 0 ){
			cout	<< cpuHead->pLink->arrivalTime << "\t"
					<< cpuHead->pLink->startTime << "\t";
		}
}

void printMetricsData(){
   // print metrics data
   cout << endl << endl;
   cout << "avgTurnaroundTime(): " << getAvgTurnaroundTime() << endl;
   cout << "getTotalThroughput(): " << getTotalThroughput() << endl;
   cout << "getCpuUtil(): " << getCpuUtil() << endl;
   cout << "getAvgNumProcInQ(): " << getAvgNumProcInQ() << endl;

   cout << endl << endl;
}

void generate_report(){}
