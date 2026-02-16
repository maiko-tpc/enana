#ifndef ANALYSIS_HPP_  // include guard
#define ANALYSIS_HPP_

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <fstream>
#include <signal.h>
#include <string.h>
#include <vector>
#include <ncurses.h>
#include <time.h>

#include "TSystem.h"
#include <TFile.h>
#include <TTree.h>
#include <TH1.h>
#include <TH2.h>
#include <TCanvas.h>
#include <TF1.h>
#include <TLatex.h>
#include <TCutG.h>

#include <THttpServer.h>
#include <TInterpreter.h>

#include "config.h"
#include "decoder.hpp"
#include "evtdata.hpp"
#include "ana_event.hpp"

/* babirl */
extern "C"{
#include <bi-config.h>
#include <bi-common.h>
#include <ridf.h>
}
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/sem.h>

using namespace std;

const int seg_v792 = 21;
const int seg_v1190 = 24;
const int seg_madc32 = 32;
const int seg_mdpp16 = 33;

class analysis{
public:
  analysis();
  ~analysis();

  void SetPID(int pidnum);

  void set_ana();
  

  void SetUseage();
  void UnsetUseage();
  bool GetUseage();
  int OpenRIDF(string fname);
  void ClearRIDFError();
  bool IsRIDFeof();
  void SetOnline();
  int  GetOnline();  
  void SetLast();
  void SetRefresh();
  void SetWeb();
  int  GetWeb();  

  void CloseRIDF();
  void analyze();
  int decode();    
  int show_comment(unsigned int comsize);
  int dec_event(unsigned int evesize);
  int dec_segment(unsigned int segsize);
  void SetROOTFile(char *fname);
  char* GetROOTFile();
  int MakeROOT();
  int CloseROOT();

  // defined in histdef.cpp
  void HistDef();
  void HistFill();
  
  // defined in treedef.cpp
  void TreeDef();

  // defined in httpdef.cpp
  void MakeTHttp(int portnum);
  void HttpHistReset();  

  // defined in ana_event.cpp
  void ana_event();
  void init_event();
  int ana_madc32();
  int ana_mdpp16();
  int cnt_v1190();

  void ana_ssd();
  void ana_rf();
  void ana_pl();
  
  // online mode read shared memory
  void SetBabianSharedMemory();
  int GetBabianSharedMemory();
  int GetSharedMemory();
  unsigned int ri(unsigned short *lbuf, unsigned int *lrp);
  void analyze_online();
  int dec_event_online(unsigned short *buf, unsigned int evesize);
  int dec_segment_online(unsigned int segsize, unsigned short *buf);

private:
  int pid;

  ifstream ridf;  
  
  // RIDF data
  unsigned int buf_header[2];
  unsigned int rev, layer, cid, blksize, address;
  unsigned int blk, eve;
  
  // shared memory
  struct sembuf semb;
  union semun semunion;
  char *shmp, *fshmp;
  unsigned int blocknum, tblocknum;
  int shmid, semid;
  unsigned short sval;
  unsigned int global_blksize;
  RIDFRHD rhdl[3]; /* RIDFRHD for Layer 0, 1, 2 */
  static const unsigned int HEADER_SIZE = 4;
  
  // output ROOT file
  TFile *outroot;
  
  TTree *tree;

  THttpServer* serv;

  // PID file
  TCutG *pid_f2;
  TCutG *pid_f3;  

  // output histograms
  TH1F *hmadc[N_MADC_CH];
  TH1F *hmadc_total;
  TH1F *hppac_good[N_PPAC];
  TH2F *hppac_pos2d[N_PPAC];
  TH2F *hppac_pos2d_cal[N_PPAC];  

  TH2F *hppac_track[2][2]; // [F2/F3][X/Y]
  TH2F *hppac_track_pid[2][2]; // [F2/F3][X/Y]  
  
  TH1F *h_f2_pos[2]; // F2 viewer position
  
  TH1F *h_attpc_x;
  TH1F *h_attpc_y;  
  TH2F *h_attpc_xy;
  
  TH1F *hssd_ene[N_SSD];

  TH2F *hpid_f2;
  TH2F *hpid_f3;  

  TH1F *h_tof_f3ppac[4];
  
  // list of histograms
  vector<TH1F*> vec_th1;
  vector<TH2F*> vec_th2;  
  
  enum RIDF_CID{
  RIDF_EF_BLOCK,
  RIDF_EA_BLOCK,
  RIDF_EAEF_BLOCK,
  RIDF_EVENT,
  RIDF_SEGMENT,
  RIDF_COMMENT,
  RIDF_EVENT_TS,
  RIDF_BLOCK_NUMBER=8,
  RIDF_END_BLOCK,
  RIDF_NCSCALER=11,
  RIDF_CSCALER=12,
  RIDF_NCSCALER32,
  RIDF_TIMESTAMP=16,
  RIDF_STATUS=21,
};

  int segdatasize,scrdatasize;  

  typedef struct{
    char name[100];
    char num[100];
    char start[20];
    char stop[20];
    char date[60];
    char comment[200];
  } RunInfo;
  
  typedef struct{
    unsigned int rev;
    unsigned int dev;    
    unsigned int fp;    
    unsigned int det;
    unsigned int mod;    
  } segmentID;


  evtdata evt;
  optdata opt;

  ana_ppac ppac[N_PPAC];
  
};

#endif
