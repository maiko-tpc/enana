#ifndef EVTDATA_H_
#define EVTDATA_H_

#include <vector>

const int N_MADC = 1;
const int N_MADC_CH = 32*N_MADC;

const int N_PPAC = 4;
const int N_PPAC_CH = 5;

struct optdata{
  char bldfname[256];
  char rootfname[256];
  int online_flag;
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


typedef struct madc32_data{
  unsigned int adc[N_MADC_CH];
  unsigned long int counter[N_MADC];
} madc32_data;

struct evtdata{
  std::vector<v1190_hit> v1190_hit_all;
  std::vector<mxdc32_hit> mxdc32_hit_all;  

  madc32_data madc;

  unsigned int eve;

  bool ppac_good[N_PPAC];
  int ppac_tdc[N_PPAC][N_PPAC_CH];
  int ppac_pos_raw[N_PPAC][2];  
  
};


#endif
