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
   struct procListNode* pLink;    // the target process
};

struct readyQNode{
   struct procListNode* pLink;    // point to matching process in process list 
   struct readyQNode* rNext;      // point to next process in the ready queue
};

struct eventQNode{
   float time;          
   int type;                      // 1 = arrival; 2 = departure; 
											 // 3 = allocation; 4 = preemption   
   struct eventQNode* eNext;      // point to next event in the event queue
   struct procListNode* pLink;    // point to matching process in process list
};

struct procListNode{
   float arrivalTime;    
   float startTime;
	float reStartTime;
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
float quantumClock;					// a counter for quantums
int batchSize;                   // argv[5] from console
float mu;                        // 1/avgTs
eventQNode* eHead;               // head of event queue
procListNode* pHead;             // head of oprocess list
readyQNode* rHead;               // head of ready queue
cpuNode* cpuHead;                // the cpu node (there's only one)

/////////////////////////////////////////////////////
// helper function
void insertIntoEventQ(eventQNode*);
float getResponseRatioValue(procListNode*);
float getQuantumStart(procListNode*);
procListNode* getSRTProcess();
procListNode* getHRRProcess();

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

	// initialize quantumClock to 0
	quantumClock = 0.0;

	// create the cpu node
   cpuHead = new cpuNode; 
   cpuHead->simClock = 0.0;
   cpuHead->cpuIsBusy = 0;   // cpu flag: 0=false=idle, 1=true=busy 
   cpuHead->pLink = 0;
       

   // create process list node, point pHead to it, initialize member vars
   // this first node is being initialized as the first process for the sim
   pHead = new procListNode;
   pHead->arrivalTime = genExp((float)lambda);   
   pHead->startTime = 0.0;
   pHead->reStartTime = 0.0;
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
	scheduleAllocation();
	handleAllocation();
	schedulePreemption();
	handlePreemption();
	handleArrival();

	//printQueues();
}

void FCFS(){
	int departureCount = 0;
	while( departureCount < batchSize ){ 		  //tester(); break; // XXXXXXXXXXXXXXX
		// CASE 1: cpu is not busy -------------------
		if( cpuHead->cpuIsBusy == false ){
			scheduleArrival();
			if( rHead != 0 ){
				scheduleAllocation();
			}
		}
		// CASE 2: cpu is busy -----------------------
		else scheduleDeparture();

		// ANY CASE: handle next event ---------------
		if( eHead->type == 1 ) handleArrival();			
		else if( eHead->type == 2 ){  
			handleDeparture();
			departureCount++;
		}
		else if( eHead->type == 3 ) handleAllocation();
	} // end while
}

void SRTF(){
	int arrivalCount = 0;
	int departureCount = 0;
	while( departureCount < batchSize ){ 		  
		// on each pass, schedule an arrival, up to batchSize * 1.20
		if( arrivalCount < ( batchSize * 1.20 ) ){
			scheduleArrival();
			arrivalCount++;
		}
		// CASE 1: cpu is not busy -------------------
		if( cpuHead->cpuIsBusy == false ){
			if( rHead != 0 ) scheduleAllocation();					
		}
		// CASE 2: cpu is busy -----------------------
		else{
			// if( okayToDepart ) scheduleDeparture();				
			if( eHead->type == 1 ){							
				// if arrival time > cpu finish time, it's ok to depart
				if( eHead->time > cpuEstFinishTime() )	
					scheduleDeparture();
				// if arrival preempts cpu, then schedule a preemption
				else if( isPreemptive() ) 
					schedulePreemption();
			}
		}
		// ANY CASE: handle next event ---------------
		if( eHead->type == 1) handleArrival(); 
		else if( eHead->type == 2 ){						
			handleDeparture();
			//okayToDepart = false;
			departureCount++;
		}
		else if( eHead->type == 3 ) handleAllocation();
		else if( eHead->type == 4 ) handlePreemption();
	} // end while
}

void HRRN(){
	int departureCount = 0;
	while( departureCount < batchSize ){ 		  
		// CASE 1: cpu is not busy -------------------
		if( cpuHead->cpuIsBusy == false ){
			scheduleArrival();
			if( rHead != 0 ){
				scheduleAllocation(); // HRR rules are in the method
			}
		}
		// CASE 2: cpu is busy -----------------------
		else scheduleDeparture();

		// ANY CASE: handle next event ---------------
		if( eHead->type == 1 ) handleArrival();			
		else if( eHead->type == 2 ){  
			handleDeparture();
			departureCount++;
		}
		else if( eHead->type == 3 ) handleAllocation();
	} // end while

}

void RR(){
	int departureCount = 0;
	while( departureCount < batchSize ){ 		  tester(); break; // XXXXXXXXXXXXXXX
		// CASE 1: cpu is not busy -------------------
		if( cpuHead->cpuIsBusy == false ){
			scheduleArrival();
			if( rHead != 0 ){
				scheduleAllocation();
			}
		}
		// CASE 2: cpu is busy -----------------------
		else{
			if( ( quantumClock + quantum ) > 
					cpuEstFinishTime() ){
			scheduleDeparture();
			}
			else{
				schedulePreemption();
			} 
		}
		// ANY CASE: handle next event ---------------
		if( eHead->type == 1 ) handleArrival();			
		else if( eHead->type == 2 ){  
			handleDeparture();
			departureCount++;
		}
		else if( eHead->type == 3 ) handleAllocation();
		else if( eHead->type == 4 ) handlePreemption();
	} // end while
}

float cpuEstFinishTime(){
	float estFinish;
	float startTime = cpuHead->pLink->startTime;
	float reStartTime = cpuHead->pLink->reStartTime;
	float remainingTime = cpuHead->pLink->remainingTime;

	if( reStartTime == 0 ) estFinish = startTime + remainingTime;
	else estFinish = reStartTime + remainingTime;

	return estFinish;
}

bool isPreemptive(){ 
	// get estimated cpu finish time
	float cpuFinishTime = cpuEstFinishTime();

	// get estimated remaining time if cpu proc is preempted
	float cpuRemainingTime =	cpuFinishTime - eHead->time;
	
	// if arrival time < cpuFinishTime && arrival's 
	// remaining time < cpuRemainingTime, then arrival is preemptive
	if( ( eHead->time < cpuFinishTime ) && 
		 ( eHead->pLink->remainingTime < 
		   cpuRemainingTime ) ){
		return true;
	}
	else return false;
}

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
   pIt->pNext->reStartTime = 0.0;
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

// schedules an allocation event in eventQ
void scheduleAllocation(){
	cout << "schedule allocation" << endl;

	// create a new event queue node
	eventQNode* nuAllocation = new eventQNode;
		
	// identify the next process to be allocated to the cpu --
	procListNode* nextProc;
	if( schedulerType == 1 ||                          // FCFS
		 schedulerType == 4 ){                          // RR
		nextProc = rHead->pLink;  
	}
	else if( schedulerType == 2 ){							// SRTF
		if( cpuHead->simClock > 
			 rHead->pLink->arrivalTime ){
			nextProc = getSRTProcess();							
		}
		else{ 
			nextProc = rHead->pLink;
		}
	}
	else if( schedulerType == 3 ){							// HRRN
		nextProc = getHRRProcess();
	}

	// set the time of the allocation event ------------------
	if( schedulerType == 1 ||									// FCFS
		 schedulerType == 2 ||									// SRTF		
		 schedulerType == 3 ){								   // HRRN
		if( cpuHead->simClock < nextProc->arrivalTime ){
			nuAllocation->time = nextProc->arrivalTime;
		} 
		else{
			nuAllocation->time = cpuHead->simClock;
		}
	}
	if( schedulerType == 4 ){                           // RR
		nuAllocation->time = 
			getQuantumStart( nextProc );
	}

	// set the values for type, next, and pLink --------------        
	nuAllocation->type = 3;                             // ALL
	nuAllocation->eNext = 0;                            // ALL
	nuAllocation->pLink = nextProc;                     // ALL

	// insert new event into eventQ
	insertIntoEventQ( nuAllocation );
}

float getQuantumStart( procListNode* nextProc ){
	float arrivalTime = nextProc->arrivalTime;
	while( quantumClock < arrivalTime ){
		quantumClock += quantum;
	}
	return quantumClock;
}

// moves process from readyQ to CPU
void handleAllocation(){
	cout << "handle allocation" << endl;

	// point cpu to the proc named in the allocation event
	cpuHead->pLink = eHead->pLink;		

	if( schedulerType == 2 ||		// SRTF
		 schedulerType == 3 ){	   // HRRN
		// find the corresponding process in readyQ and move
		// it to top of readyQ if it's not already there
		readyQNode* rIt = rHead->rNext;
		readyQNode* rItPrev = rHead;
		if( rItPrev->pLink->arrivalTime !=
			 eHead->pLink->arrivalTime ){
			while( rIt != 0 ){
				if( rIt->pLink->arrivalTime ==
					 eHead->pLink->arrivalTime ){
					rItPrev->rNext = rIt->rNext;
					rIt->rNext = rHead;
					rHead = rIt;
					break;
				}
				rIt = rIt->rNext;
				rItPrev = rItPrev->rNext;
			}
		}
	}
	
	// pop the readyQ and eventQ records
	popReadyQHead();							
	popEventQHead();			

	// set the busy flag to show the cpu is now busy
	cpuHead->cpuIsBusy = true;				
	
	// update sim clock
	if( schedulerType == 4 ){
		if( cpuHead->simClock < quantumClock ){
			cpuHead->simClock = quantumClock;
		}
	}
	else{
		if( cpuHead->simClock < cpuHead->pLink->arrivalTime ){		
			// if clock < arrival time, then clock = arrival time
			cpuHead->simClock = cpuHead->pLink->arrivalTime;
		}
	}

	// update start/restart time as needed
	if( cpuHead->pLink->startTime == 0 ){  
		cpuHead->pLink->startTime = cpuHead->simClock;
	}
	else{
		cpuHead->pLink->reStartTime = cpuHead->simClock;
	}
}

// schedules a departure event in eventQ
void scheduleDeparture(){
	cout << "schedule departure" << endl;

	// create a new event node for the departure event     
   eventQNode* nuDeparture = new eventQNode;
   nuDeparture->type = 2;
   nuDeparture->eNext = 0;
   nuDeparture->pLink = cpuHead->pLink;

	// set the departure time for the event ------------
	if( schedulerType == 1 ||							// FCFS
		 schedulerType == 3 ){							// HRRN
		nuDeparture->time =	
			cpuHead->pLink->startTime + 
			cpuHead->pLink->remainingTime;
	}
	else if( schedulerType == 2 ||					// SRTF
				schedulerType == 4 ){					// RR
		if( cpuHead->pLink->reStartTime == 0 ){
			nuDeparture->time =	
				cpuHead->pLink->startTime + 
				cpuHead->pLink->remainingTime;	
		}
		else{ 
			nuDeparture->time =							
				cpuHead->pLink->reStartTime + 
				cpuHead->pLink->remainingTime;	
		}
	}

	// insert the new event into eventQ in asc time order
	insertIntoEventQ(nuDeparture);
}

// terminates a process and clears the cpu
void handleDeparture(){
	cout << "handle departure" << endl;

   // update cpu data
	cpuHead->pLink->finishTime = eHead->time;	
	cpuHead->pLink->remainingTime = 0.0;	
	cpuHead->pLink = 0;								
   cpuHead->cpuIsBusy = false;

	// update the simClock for RR 
	if( schedulerType == 4 ){
		while( quantumClock < eHead->time ){
			quantumClock += quantum;
		}
		cpuHead->simClock = quantumClock;	
	} 
	// update the simClock for FCFS, SRTF, HRRN
	else{
		cpuHead->simClock = eHead->time;	
	}

	// pop the departure from the eventQ
	popEventQHead();
}

// schedules a preemption event in eventQ
void schedulePreemption(){
	cout << "schedule preemption" << endl;

	eventQNode* nuPreemption = new eventQNode;
	if( schedulerType == 4 ){ 
		nuPreemption->time = quantumClock + quantum;
		nuPreemption->pLink = rHead->pLink;
	}
	else{
		nuPreemption->time = eHead->time;
		nuPreemption->pLink = eHead->pLink;
	}
	nuPreemption->type = 4;
	nuPreemption->eNext = 0;


	// pop the arrival event from eHead so that 
	// it can be replaced by the preemption event
	popEventQHead();

	// insert new event into eventQ
	insertIntoEventQ( nuPreemption );
}

// moves a preempting proc from readyQ to the cpu
void handlePreemption(){
	cout << "handle preemption" << endl;

	// create a temp ptr to hold the current cpu pLink
	procListNode* preemptedProcPtr = cpuHead->pLink;

	// update the remaining time
	if( schedulerType == 4 ){
		cpuHead->pLink->remainingTime =	
			cpuHead->pLink->remainingTime - quantum;			
	}
	else{
		cpuHead->pLink->remainingTime =	
			cpuEstFinishTime() - eHead->time;
	}

	// point cpu to preempting process and update data as needed
	cpuHead->pLink = eHead->pLink;
	cpuHead->simClock = eHead->time;
	if( cpuHead->pLink->reStartTime == 0.0  ){
		cpuHead->pLink->startTime = eHead->time;
	}
	else{
		cpuHead->pLink->reStartTime = eHead->time;
	}

	// schedule an arrival event for the preempted proc
	eventQNode* preemptedProcArrival = new eventQNode;
	preemptedProcArrival->time = eHead->time;
	preemptedProcArrival->type = 1;
	preemptedProcArrival->eNext = 0;
	preemptedProcArrival->pLink = preemptedProcPtr;

	// pop the preemption event from the eventQ
	popEventQHead();

	// pop the top off readyQ if doing Round Robin
	if( schedulerType == 4 ){
		popReadyQHead();
	}

	// insert new event into eventQ
	insertIntoEventQ( preemptedProcArrival );		
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

void insertIntoEventQ( eventQNode* nuEvent ){
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
				break;
         }             
         else if( (eIt->time < nuEvent->time) && 
                  (eIt->eNext->time > nuEvent->time)){
            nuEvent->eNext = eIt->eNext;
            eIt->eNext = nuEvent;     
				break;
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

procListNode* getSRTProcess(){ // DONE
	// pre-condition: there is something in the readyQ
	readyQNode* rIt = rHead;
	procListNode* srtProc = rIt->pLink;
	float srt = rIt->pLink->remainingTime;
	while( rIt != 0){
		if( rIt->pLink->remainingTime < srt ){
			srt = rIt->pLink->remainingTime;
			srtProc = rIt->pLink;
		}
		rIt = rIt->rNext;
	}
	return srtProc;
}

procListNode* getHRRProcess(){
	readyQNode* rIt = rHead;
	procListNode* hrrProc = rIt->pLink;
	float hrr = getResponseRatioValue( hrrProc );		

	while( rIt != 0){
		if( getResponseRatioValue( rIt->pLink ) > hrr ){
			hrr = getResponseRatioValue( rIt->pLink );
			hrrProc = rIt->pLink;
		}
		rIt = rIt->rNext;
	}
	return hrrProc;
}

float getResponseRatioValue( procListNode* thisProc ){
	float HRR = ( ( cpuHead->simClock - 
		             thisProc->arrivalTime ) +
					    thisProc->serviceTime ) /
						 thisProc->serviceTime;
	return HRR;
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
   cout << "arrivalTime\t\t serviceTime\t\t sTime\t\t rsTime\t\t fTime\t\t rTime\t\t" << endl;
   while( pIt != 0 ){
      cout << pIt->arrivalTime << "  \t\t"
           << pIt->serviceTime << "  \t\t"
           << pIt->startTime   << "  \t\t"
           << pIt->reStartTime   << "  \t\t"
           << pIt->finishTime  << "  \t\t"
           << pIt->remainingTime  << "  \t\t"
           << endl;
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
				<< cpuHead->cpuIsBusy << "\t\t";
		if( cpuHead->pLink != 0 ){
			cout	<< cpuHead->pLink->arrivalTime << "\t"
					<< cpuHead->pLink->startTime << "\t"
					<< endl;   
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
