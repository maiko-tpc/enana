#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <arpa/inet.h>
#include "decoder.hpp"

const int V1190_REF_CH = 127;
const int N_V1190_CH = 128;
const int V1190_MAX_MULTI = 128;
const int V1190_SUBTRACT_REF = 1;

int ana_v1190(unsigned int size, unsigned int *tmpdata, evtdata *evt){
  unsigned int wrdcnt=6;
  unsigned int geo=0;
  unsigned int data;
  unsigned int edge, ich, measure;

  int tmp_lead[N_V1190_CH][V1190_MAX_MULTI]={0};   // 128ch
  int tmp_trail[N_V1190_CH][V1190_MAX_MULTI]={0};  // 128ch
  int lead_cnt[N_V1190_CH]={0};
  int trail_cnt[N_V1190_CH]={0};

  int ref_lead=0;
  int ref_flag=0;
  
  //  printf("---\n");
  int i=0;
  while(1){
    data = tmpdata[i];
    i++;
    wrdcnt+=2;
    
    //    printf("v1190 data: 0x%08x\n", data);

    if((data>>27)  == 0x08){  // global header
      geo=data&0x1f;
      //      printf("geo=%d\n", geo);
    }

    if((data>>27) == 0x1){  // TDC header
    }

    if((data>>27) == 0x03){ // TDC trailer
    }

    if((data>>27) == 0x4){ // TDC error
    }
    
    if((data>>27) == 0x00){ // TDC measure
      edge=(data>>26)&0x3f;
      ich=(data>>19)&0x7f;
      measure=data&0x0007ffff;

      if(edge==0){ // leading edge
	if(lead_cnt[ich]<V1190_MAX_MULTI){
	  tmp_lead[ich][lead_cnt[ich]]=measure;
	  //              if(ich==ref_ch && ref_flag[geo]==0){
	  if(ich==V1190_REF_CH && ref_flag==0){
	    ref_lead=measure;
	    ref_flag=1;
	  }
	  lead_cnt[ich]++;
	}
	
      }
      if(edge==1){ // trailing edge
	if(trail_cnt[ich]<V1190_MAX_MULTI){
	  tmp_trail[ich][trail_cnt[ich]]=measure;
	  trail_cnt[ich]++;
	}
	
      } // end of edge==1
    } // end of TDC measure

    if(data>>27 == 0x10) break; // global trailer

    if(wrdcnt>=size) break;

  } // end of while(1)


  // reanalysis
  int total_hit=0;
  v1190_hit tmp_hit;

  //  if(ref_flag==1){
  //  printf("--\n");
  if(1){    
    for(int i=0; i<N_V1190_CH; i++){
      if(lead_cnt[i]>0){
	for(int j=0; j<lead_cnt[i]; j++){
	  
	  tmp_hit.geo=geo;
	  tmp_hit.ch=i;
	  tmp_hit.counter=0;
	  tmp_hit.lead_raw = tmp_lead[i][j];
	  tmp_hit.trail_raw=0;
	  tmp_hit.lead_cor = tmp_lead[i][j];
	  tmp_hit.tot=0;
	  if(V1190_SUBTRACT_REF==1){
	    tmp_hit.lead_cor -= ref_lead;
	  }
	  
	  if(lead_cnt[i]==trail_cnt[i]){
	    tmp_hit.trail_raw = tmp_trail[i][j];
	    tmp_hit.tot = tmp_trail[i][j] - tmp_lead[i][j];
	  }
	  else{
	    tmp_hit.trail_raw = -1;
	    tmp_hit.tot = -1;
	  }
	  
	  //	  	  printf("geo:%d, ch:%d, lead:%d\n",
	  //	  		 tmp_hit.geo, tmp_hit.ch, tmp_hit.lead_raw);
	  
	  evt->v1190_hit_all.push_back(tmp_hit);
	  total_hit++;

	}
      }
    } // end of multi-hit loop
  } // end of ref_flag

  return 0;
}


int ana_mxdc32(vector<mxdc32_hit> &mxdc32_hit_all, unsigned int size,
	     unsigned int *tmpdata){

  unsigned int rp=0;
  unsigned int data;
  int geo=0;
  int nword;
  int ich;
  unsigned int tmpadc;
  unsigned long int tmp_counter=0;
  mxdc32_hit tmp_hit;

  int no_header=1;
  int no_ender=1;  

  unsigned int word_cnt = 6;
  

  while(1){
    data = tmpdata[rp];
    rp++;
    word_cnt +=2;
    
    if((data>>24) == 0x40){  // header
      geo=(data>>16)&0x00ff;
      nword=(data)&0x00000fff;
      no_header=0;
      
      for(int i=0; i<nword-1; i++){
	data = tmpdata[rp];
	rp++;

	if(((data>>21) & 0x7ff) ==0x20){ // ADC data words
          ich = (data>>16)&0x001f;
          tmpadc=(data)&0x00001fff;
          tmp_hit.field = 0;
          tmp_hit.geo = geo;
          tmp_hit.ch = ich;
          tmp_hit.adc = tmpadc;
          mxdc32_hit_all.push_back(tmp_hit);
        }	

	if(((data>>21) & 0x7ff) ==0x24){ // extended time stamp
	  tmp_counter |= (data&0xffff)<<30;
	}
      }
    }

    if(((data>>30))==0x3){  // ender
      tmp_counter |= (data&0x3fffffff);
      no_ender=0;
      
      // counter = ch33
      tmp_hit.field = 0;
      tmp_hit.geo = geo;
      tmp_hit.ch = 32;
      tmp_hit.adc = tmp_counter;
      mxdc32_hit_all.push_back(tmp_hit);
      
      break;
    }
    
    if(word_cnt >= size) break;
  }
  
  if(no_header==1) printf("MADC32 no header\n");
  if(no_ender==1)  printf("MADC32 no ender:\n");  

  return 0;
}
