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
  double ang[2];  // X and Y angle

  
private:  
  int ippac;

  //  double cal_par_p0[4][2] = {{3.3161,-4.476}, {-0.5834,-2.755},
  //  			     {-0.6065,1.259} , {0.8924,0.0785}};

  //add beam center position at ppac by sakajo
  //  double cal_par_p0[4][2] = {{3.3161,-4.476}, {-0.5834,-2.755},
  //  			     {-2.1065,-0.041} , {-0.2076,1.3785}};
  // new one = past one - cal_par_bc
  //  double cal_par_bc[4][2] = {{0.0, 0.0}, {0.0, 0.0},
  //			     {1.5, 1.3}, {1.1, -1.3}};

  double cal_par_p0[4][2] = {{3.3161,-4.476}, {-0.5834,-2.755},
			     {-1.5, -1.3}, {-1.1, 1.3}};

  //  double cal_par_p1[4][2] = {{0.0527,0.0511}, {0.0555, 0.0539},
  //			     {0.0577, 0.0583}, {0.0576,0.0568}};  // original calib by furuno

  double cal_par_p1[4][2] = {{0.062,0.061}, {0.061, 0.061},
			     {0.0577, 0.0583}, {0.0576,0.0568}};  

  double offset[4][2] = {{-0.74,2.72}, {0.93,1.26},
			 {1.08,-3.96}, {0.01,-3.02}};

};

#endif
