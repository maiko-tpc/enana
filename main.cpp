#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <fstream>
#include <signal.h>
#include <string.h>

#include <vector>

#include <TFile.h>
#include <TTree.h>
#include <TH1F.h>
#include <TH2F.h>
#include <THttpServer.h>

#include "analysis.hpp"

using namespace std;

// signal handler
void abrt_handler(int sig, siginfo_t *info, void *ctx);
volatile sig_atomic_t eflag;
int STOP_FLAG=0;

void clear_handler(int sig);
int CLEAR_FLAG=0;

int main(int argc, char *argv[]) {

  /* define action when Cntl-c is given */
  struct sigaction sa_sigabrt;
  memset(&sa_sigabrt, 0, sizeof(sa_sigabrt));
  sa_sigabrt.sa_sigaction = abrt_handler;
  sa_sigabrt.sa_flags = SA_SIGINFO;

  if (sigaction(SIGINT, &sa_sigabrt, NULL) < 0 ) {
    exit(1);
  }

  /* Define action for clear button */
  signal(SIGUSR2, clear_handler);  // SIGID=12 (user definition signal2)

  
  /* Get pid of myself */
  pid_t my_pid = getpid();
  printf("PID: %d\n", (int)my_pid);
  
  /* main analysis frame */
  analysis *ana = new analysis();
  ana->SetPID(my_pid);
  
  // analysis of command line options
  int portnum=5920;
  string infile_name;
  for(int i=0; i<argc; i++){

    // show help
    if(strstr(argv[i], "-h") != NULL){
      ana->SetUseage();
      printf("hoge\n");
    }
    
    // input file
    if(strstr(argv[i], ".ridf") != NULL){
      infile_name = argv[i];
      ana->UnsetUseage();
    }
    if(strstr(argv[i], ".ridf") == NULL){
      ana->UnsetUseage();
    }

   // output file
    if(strstr(argv[i], ".root") != NULL){
      ana->SetROOTFile(argv[i]);
    }
    
    // online flag
    if(strstr(argv[i], "-online") != NULL){
      ana->SetOnline();
    }

    // last flag
    if(strstr(argv[i], "-last") != NULL){
      ana->SetLast();
    }

    // refresh flag
    if(strstr(argv[i], "-refresh") != NULL){
      ana->SetRefresh();
    }

    // THttp flag
    if(strstr(argv[i], "-web") != NULL){
      portnum=atoi(argv[i+1]);
      ana->SetWeb();
      ana->SetOnline();
    }

    // Read Sheard Memory && THttp flag
    if(strstr(argv[i], "-babian") != NULL){
      portnum=atoi(argv[i+1]);
      ana->SetWeb();
      ana->SetBabianSharedMemory();
    }

  } // end of option analysis

  /* Usage */
  if(ana->GetUseage()==1 || argc==1){
    printf("\n");
    printf("Usage: ./enana xxxx.ridf xxxx.root\n");
    printf("\n");
    exit(1);
  }
  
  /* open RIDF file or shared memory */
  if (ana->GetBabianSharedMemory() == 1){
    /* Online mode read shared memory */
    printf("Online analysis mode Read babian shared memory.\n");
    ana->GetSharedMemory();
  }else{
    /* input RIDF file */
    if (ana->OpenRIDF(infile_name)!=0) exit(1);
  }
  
  /* output ROOT file */
  ana->MakeROOT();
  ana->HistDef();
  ana->TreeDef();  
  if(ana->GetWeb()) ana->MakeTHttp(portnum);
  
  
  /* main analysis routine */
  if (ana->GetBabianSharedMemory() == 1){
    /* Online analysis mode -Babian Shared Memory- */
    ana->analyze_online();
  }else{
    /* Offline analysis mode */
    ana->analyze();
    ana->CloseRIDF();
  }
  ana->CloseROOT();
  
  
  return 0;
}

void abrt_handler(int sig, siginfo_t *info, void *ctx) {
  printf("\n");
  printf("Interrupt is detected!\n");
  printf("\n");
  eflag = 1;
  STOP_FLAG=1;
  //  exit(0);
}

void clear_handler(int signal){
  printf("clear signal!\n");
  CLEAR_FLAG = 1;
}
