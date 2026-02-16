#ifndef EVTDATA_H_
#define EVTDATA_H_

#include <vector>

const int N_MADC = 1;
const int N_MADC_CH = 32*N_MADC;
const int N_MDPP = 1;
const int N_MDPP_CH = 16*N_MDPP;

const int N_PPAC = 4;
const int N_PPAC_CH = 5;

const int N_SSD = 2;
const int N_RF = 4;

const int N_PL_CH = 4;

struct optdata{
  char bldfname[256];
  char rootfname[256];
  int online_flag;
  int babian_flag;
  int last_flag;
  int refresh_flag;
  int web_flag;
  int useage_flag;
  char parfname[256];
};

typedef struct v1190_hit{
  unsigned int geo;
  unsigned int ch;
  int lead_raw;
  int trail_raw;
  int lead_cor;
  int tot;
  unsigned int counter;
} v1190_hit;

typedef struct mxdc32_hit{
  int field;
  unsigned int geo;
  unsigned int ch;
  unsigned long int adc;
} mxdc32_hit;

typedef struct mdpp32_hit{
  int field;
  unsigned int geo;
  unsigned int ch;
  unsigned long int adc;
} mdpp16_hit;


typedef struct madc32_data{
  unsigned int adc[N_MADC_CH];
  unsigned long int counter[N_MADC];
} madc32_data;

typedef struct mdpp16_data{
  unsigned int adc[N_MDPP_CH];
  unsigned long int counter[N_MDPP];
} mdpp16_data;

typedef struct ssd_data{
  int adc[N_SSD];
  float ene[N_SSD];
  int tdc[N_SSD];
} ssd_data;

struct evtdata{
  std::vector<v1190_hit> v1190_hit_all;
  std::vector<mxdc32_hit> mxdc32_hit_all;
  std::vector<mdpp16_hit> mdpp16_hit_all;    

  int sleep_cnt;
  
  madc32_data madc;
  unsigned long int counter_prev;

  mdpp16_data mdpp;  

  unsigned int eve;

  int tdc_hit[128];
  int tdc_multi[128];
  int tdc_lead[128];    
  
  bool ppac_good[N_PPAC];
  int ppac_tdc[N_PPAC][N_PPAC_CH];
  int ppac_pos_raw[N_PPAC][2];  
  float ppac_pos_cal[N_PPAC][2];
  float ppac_ang[2][2];
  float f2_pos[2];
  
  double ssd_ene[2];

  ssd_data ssd;

  int ref_tdc;
  int rf[N_RF];

  int pl_tdc[N_PL_CH];

  bool use_pid_f2;
  bool use_pid_f3;
  bool is_pid_f2;
  bool is_pid_f3;
  
};


#endif
