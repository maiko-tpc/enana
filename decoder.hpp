#ifndef DECODER_HPP_
#define DECODER_HPP_

#include "evtdata.hpp"

using namespace std;

const int MAXSIZE=100000;

int ana_v1190(unsigned int size, unsigned int *tmpdata, evtdata *evt);

int ana_mxdc32(vector<mxdc32_hit> &mxdc32_hit_all, unsigned int size,
	     unsigned int *tmpdata);

#endif
