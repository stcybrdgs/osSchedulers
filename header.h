/****************************************************
*   course:       CS4328 
*   project:      Project 1 - Scheduler Simulator 
*   programmer:   Stacy Bridges
*   date:         10/29/2015  
*   description:  for description, see "main.cpp"
****************************************************/
#ifndef HEADER_H
#define HEADER_H

/////////////////////////////////////////////////////
// utility functions
void parseArgs(char *[]);  
void init();
void run_sim();
float urand();
float genExp(float);
void generate_report();

/////////////////////////////////////////////////////
// scheduler functions
void FCFS();   // first come first serve 
void SRTF();   // shortest remaining time next  
void HRRN();   // highest response ratio next
void RR();     // round robin 

/////////////////////////////////////////////////////
// helper functions
void scheduleArrival();
void scheduleDeparture();
void scheduleAllocation();
void schedulePreemption();
void handleArrival();
void handleDeparture();
void handleAllocation();
void handlePreemption();
void popReadyQHead();
void popEventQHead();
bool isPreempted();
float cpuEstFinishTime();

/////////////////////////////////////////////////////
// metric computation functions
float getAvgTurnaroundTime();
float getTotalThroughput();
float getCpuUtil();
float getAvgNumProcInQ();

/////////////////////////////////////////////////////
// test functions
void printQueues();   
void printMetricsData();
void tester();

#endif