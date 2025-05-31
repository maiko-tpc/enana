#ifndef ANA_EVENT_HPP_  // include guard
#define ANA_EVENT_HPP_

#include "evtdata.hpp"

const int PPAC_TDC_N_CH = 5;

class ana_ppac{
public:
  ana_ppac();
  ~ana_ppac();  
  void SetPPACi(int index);
  void SetTDCGeo(unsigned int geo);
  void SetTDCCh(unsigned int *ch);
  int analyze(evtdata *evt);
  bool IsGoodHit();
  int GetPosRaw(int ixy);
  double GetPosCal(int ixy);

  unsigned int tdc_geo;
  unsigned int tdc_ch[PPAC_TDC_N_CH]; // L->D->R->U->A

  int tdc_val[PPAC_TDC_N_CH];
  int tdc_hit[PPAC_TDC_N_CH];
  int hit_bit;
  
  int pos_raw[2];
  double pos_cal[2];

  
private:  
  int ippac;
  
};

#endif
