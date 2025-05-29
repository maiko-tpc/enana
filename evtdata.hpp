#ifndef EVTDATA_H_
#define EVTDATA_H_

#include <vector>

const int N_MADC = 1;
const int N_MADC_CH = 32*N_MADC;

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

  unsigned int ppac_f2u_good, ppac_f2d_good;
  unsigned int ppac_f3u_good, ppac_f3d_good;  

  madc32_data madc;

  unsigned int eve;
};


#endif
