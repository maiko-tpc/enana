#include "analysis.hpp"
#include "ana_event.hpp"

analysis::analysis(){
  evt.eve=0;

  sprintf(opt.rootfname, "out.root");
  opt.online_flag=0;
  opt.last_flag=0;
  opt.refresh_flag=0;
  opt.web_flag=0;
  opt.useage_flag=1;
  sprintf(opt.parfname, "par/default.par");

  vec_th1.clear();
  vec_th2.clear();  
}

analysis::~analysis(){
}

void analysis::SetPID(int pidnum){
  pid = pidnum;
}

void analysis::set_ana(){

  // CH mapping of TDC
  int ppac_geo = 7;
  unsigned int ppac_ch[N_PPAC][5] = {{0,1,2,3,4}, {5,6,7,8,9},
				     {16,17,18,19,20}, {21,22,23,24,25}};
  
  for(int i=0; i<N_PPAC; i++){
    ppac[i].SetPPACi(i);
    ppac[i].SetTDCGeo(ppac_geo);
    ppac[i].SetTDCCh(ppac_ch[i]);
  }

}

void analysis::SetUseage(){
  opt.useage_flag=1;
}

void analysis::UnsetUseage(){
  opt.useage_flag=0;
}

bool analysis::GetUseage(){
  return opt.useage_flag;
}

int analysis::OpenRIDF(string fname){
  ridf.open(fname, ios_base::in|ios::binary);
  if(!ridf){
    printf("RIDF File '%s' does not exist!\n", fname.c_str());
    return 1;
  }
  printf("Input RIDF: %s\n", fname.c_str());
  
  return 0;
}

void analysis::ClearRIDFError(){
  ridf.clear();
}

bool analysis::IsRIDFeof(){
  return ridf.eof();
}
void analysis::CloseRIDF(){
  ridf.close();
}

void analysis::SetOnline(){
  opt.online_flag=1;
}

int analysis::GetOnline(){
  return opt.online_flag;
}

void analysis::SetLast(){
  opt.last_flag=1;
}
void analysis::SetRefresh(){
  opt.refresh_flag=1;
}
void analysis::SetWeb(){
  opt.web_flag=1;
}
int analysis::GetWeb(){
  return opt.web_flag;
}
int analysis::MakeROOT(string fname){
  outroot = new TFile(fname.c_str(), "RECREATE");
  return 0;
}

int analysis::CloseROOT(){
  tree->Write();
  outroot->Close();
  return 0;
}

int analysis::analyze(){

  blk=0;
  unsigned int tmpbuf;
  
  set_ana();
  
  while(1){
    
    // Read the header
    ridf.read((char*)buf_header, sizeof(buf_header));
    cid = (buf_header[0] & 0x0FC00000) >> 22;
    blksize= buf_header[0] & 0x003FFFFF;
    address = buf_header[1];
    //    printf("cid:%d, size:%d\n", cid, blksize);
    
    switch(cid){
    case RIDF_EF_BLOCK:  //Event Fragment
      break;
      
    case RIDF_EA_BLOCK:       // Event Assembly
      break;
      
    case RIDF_EAEF_BLOCK: // Event Assembly Fragment
      break;
      
    case RIDF_COMMENT:
      show_comment(blksize);
      break;
      
    case RIDF_END_BLOCK:       // end of block
      ridf.read((char*)&tmpbuf, sizeof(tmpbuf));      
      break;
      
    case RIDF_BLOCK_NUMBER: // Counter of the number of block
      ridf.read((char*)&tmpbuf, sizeof(tmpbuf));      
      blk = tmpbuf;
      break;
      
    case RIDF_EVENT: // event data header
      init_event();
      dec_event(blksize);
      ana_event();
      tree->Fill();
      HistFill();
      break;
      
    case RIDF_SEGMENT: // segment data
      break;
      
    default:
      // printf("Error: RIDF Class ID:%d is invalid.\n",cid);
      break;
    }
    
    // IMPORTANT: one should regularly call ProcessEvents
    // to let http server process requests
    if(gSystem->ProcessEvents()) break;
    
    if( opt.online_flag==1 && IsRIDFeof()){
      printf("Waiting for data... (eve: %d)\r",eve);
      fflush(stdout);
      ClearRIDFError();
      usleep(100000);
    }
    
    if( opt.online_flag==0 && IsRIDFeof()){
      break;
    }
  } // end of while
  
  printf("eve: %d\n", eve);  
  return 0;
}

int analysis::decode(){
  return 0;
}

int analysis::show_comment(unsigned int comsize){
  //  printf("Run comment, size=%d\n", comsize);

  RunInfo info;
  unsigned int date,comid;
  char comment[100000];
  
  unsigned int wrdcnt=4;
  
  ridf.read((char*)&date, sizeof(date));
  wrdcnt+=2;

  ridf.read((char*)&comid, sizeof(comid));
  wrdcnt+=2;  

  //  printf("date: %d, id: %d\n", date, comid);

  unsigned int readsize_byte = (comsize-wrdcnt)*2;
  ridf.read((char*)comment, readsize_byte);

  if(comid==1){  // header
    strncpy(info.name,comment,100);
    strncpy(info.num,&comment[100],100);
    strncpy(info.start,&comment[200],20);
    strncpy(info.stop,&comment[220],20);
    strncpy(info.date,&comment[240],60);
    strncpy(info.comment,&comment[300],200);

    printf("\n");
    printf("Run_Name: %s\n",info.name);
    printf("Run_Num : %s\n",info.num);
    printf("Start   : %s\n",info.start);
    printf("Stop    : %s %s\n",info.stop, info.date);
    printf("Comment : %s\n\n",info.comment);
  }
  
  return 0;
}

int analysis::dec_event(unsigned int evesize){
  unsigned int tmpbuf;
  unsigned int wrdcnt=4;
  
  // Read the event number in the header
  ridf.read((char*)&tmpbuf, sizeof(tmpbuf));      
  wrdcnt+=2;
  eve = tmpbuf;
  if(eve%10000==0){
    printf("eve: %d\r", eve);
    fflush(stdout);
  }
  
  while(wrdcnt<evesize){

    // Read the header
    ridf.read((char*)buf_header, sizeof(buf_header));
    wrdcnt+=(sizeof(buf_header))*2;
    cid = (buf_header[0] & 0x0FC00000) >> 22;
    blksize= buf_header[0] & 0x003FFFFF;
    address = buf_header[1];
    //    printf("cid:%d, size:%d\n", cid, blksize);

    switch(cid){
    case RIDF_SEGMENT:
      wrdcnt+=dec_segment(blksize);
      break;
    case RIDF_NCSCALER:
      break;
    case RIDF_CSCALER:
      break;
    case RIDF_NCSCALER32:
      break;
    default:
      printf("Error: Invalid class ID in event: %d\n",cid);
      break;

    } // end of switch(cid)

  }  // end of  while(wrdcnt<size){
  
  return 0;
}

int analysis::dec_segment(unsigned int segsize){
  unsigned int tmpbuf;
  unsigned int wrdcnt=4;
  unsigned int segid;
  unsigned int tmpdata[MAXSIZE];

  segmentID id;
  
  // Read the segment id
  ridf.read((char*)&tmpbuf, sizeof(tmpbuf));      
  wrdcnt+=2;
  segid = tmpbuf;
  //  printf("segid: 0x%x\n", segid);

  id.rev = (segid>>26)&0x3F;
  id.dev = (segid>>20)&0x3F;
  id.fp  = (segid>>14)&0x3F;
  id.det = (segid>> 8)&0x3F;
  id.mod = segid&0xFF;
  //printf("rev:%d, dev:%d, fp:%d, det:%d, mod:%d\n",
  //id.rev, id.dev, id.fp, id.det, id.mod);

  ridf.read((char*)&tmpdata, (segsize-wrdcnt)*2);        
  wrdcnt+=segsize-wrdcnt;
  
  switch(id.mod){
  case seg_v1190:
    ana_v1190(segsize, tmpdata, &evt);
    break;

  case seg_madc32:
    ana_mxdc32(evt.mxdc32_hit_all, segsize, tmpdata);
    break;

  case seg_v792:
    break;
    
  default:
    printf("Unknown Module ID: %d\n", id.mod);
    break;
    
  } // end of switch(id.mod)
  
  return wrdcnt;
}
