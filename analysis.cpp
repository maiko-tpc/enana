#include "analysis.hpp"
#include "ana_event.hpp"
// #define ONLINE_DEBUG

analysis::analysis(){
  evt.eve=0;
  evt.sleep_cnt=0;
  
  sprintf(opt.rootfname, "out.root");
  opt.online_flag=0;
  opt.babian_flag=0;
  opt.last_flag=0;
  opt.refresh_flag=0;
  opt.web_flag=0;
  opt.useage_flag=1;
  sprintf(opt.parfname, "par/default.par");

  vec_th1.clear();
  vec_th2.clear();  

  // read the PID files
  evt.use_pid_f2=0;
  evt.use_pid_f3=0;  

  TFile *file_pid_f2 = new TFile("macro/pid_f2.root");
  if(file_pid_f2->IsOpen()) pid_f2 = (TCutG*)file_pid_f2->Get("pidcut_f2");
  if(pid_f2->GetN()>2) evt.use_pid_f2=1;
  
  TFile *file_pid_f3 = new TFile("macro/pid_f3.root");
  if(file_pid_f3->IsOpen()) pid_f3 = (TCutG*)file_pid_f3->Get("pidcut_f3");
  if(pid_f3->GetN()>2) evt.use_pid_f3=1;

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
  printf("Input  RIDF: %s\n", fname.c_str());
  
  return 0;
}

void analysis::ClearRIDFError(){
  printf("clear ridf file...\n");
  ridf.clear();
}

bool analysis::IsRIDFeof(){
  return ridf.eof();
}
void analysis::CloseRIDF(){
  printf("closing ridf...\n");
  ridf.close();
}

void analysis::SetOnline(){
  opt.online_flag=1;
}

int analysis::GetOnline(){
  return opt.online_flag;
}

void analysis::SetBabianSharedMemory(){
  opt.babian_flag=1;
}
int analysis::GetBabianSharedMemory(){
  return opt.babian_flag;
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
void analysis::SetROOTFile(char *fname){
  sprintf(opt.rootfname, "%s", fname);
}
char* analysis::GetROOTFile(){
  return opt.rootfname;
}
int analysis::MakeROOT(){
  if (opt.babian_flag==1){
    outroot = new TFile("root/online.root", "RECREATE");
    printf("Output root: root/online.root\n");
    return 0;
  }else{
    outroot = new TFile(GetROOTFile(), "RECREATE");
    printf("Output root: %s\n", GetROOTFile());
    return 0;
  }
}


void analysis::analyze(){
  extern int STOP_FLAG;
  
  blk=0;
  unsigned int tmpbuf;
  
  set_ana();

  unsigned int readcnt = 0;
  
  while(1){
    
    // Read the header
    ridf.read((char*)buf_header, sizeof(buf_header));
    //    cid = (buf_header[0] & 0x0FC00000) >> 22;
    rev =   (buf_header[0]>>30) & 0x3;
    layer = (buf_header[0]>>28) & 0x3;    
    cid = (buf_header[0]>>22) & 0x3f;    
    blksize= buf_header[0] & 0x003FFFFF;
    address = buf_header[1];
    //    printf("1. rev:%d, ly:%d, cid:%d, size:%d\n", rev, layer, cid, blksize);
    
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
      
    case RIDF_NCSCALER:
      //      printf("sca\n");
      readcnt=4;
      while(readcnt<blksize){
	ridf.read((char*)&tmpbuf, sizeof(tmpbuf));      	
	readcnt+=2;
      }
      break;
    case RIDF_CSCALER:
      //      printf("csca\n");
      readcnt=4;
      while(readcnt<blksize){
	ridf.read((char*)&tmpbuf, sizeof(tmpbuf));      	
	readcnt+=2;
      }
      break;
    case RIDF_NCSCALER32:
      //      printf("sca32\n");
      readcnt=4;
      while(readcnt<blksize){
	ridf.read((char*)&tmpbuf, sizeof(tmpbuf));      	
	readcnt+=2;
      }
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
      printf("Waiting for data... (eve: %d) sleep_cnt=%d\r",evt.eve, evt.sleep_cnt);
      fflush(stdout);
      ClearRIDFError();
      //usleep(100000);
      sleep(1);
      evt.sleep_cnt++;
    }
    
    if( opt.online_flag==0 && IsRIDFeof()){
      break;
    }

    if(STOP_FLAG==1){
      printf("bye-bye!\n");
      break;
    }
  } // end of while
  
  printf("eve: %d\n", eve);  
  printf("end of ana\n");

  //  return 0;
}

int analysis::GetSharedMemory(){
  /** Open data stream **************/
  blk = 0;
  /* Shared Memory */
  if(!(shmid = initshm(ANSHMKEY,
    EB_EFBLOCK_BUFFSIZE * WORDSIZE + sizeof(blk),
    &shmp))){
      printf("Can't create shared memory\n");
      return 1;
    }
    fshmp = shmp + EB_EFBLOCK_BUFFSIZE * WORDSIZE;
    
    /* Semaphore */
    if(!(semid = initsem(ANSEMKEY, &semunion))){
      printf("Can't create semaphore\n");
      return 1;
    }
    
  return 0;
}

unsigned int analysis::ri(unsigned short *lbuf, unsigned int *lrp){
  unsigned int idat;
  idat=*(int *)(lbuf+*lrp);
  *lrp+=2;
  return(idat);
}

void analysis::analyze_online(){
  extern int STOP_FLAG;
  
  RIDFHD hd;
  unsigned short *buf;
  buf=(unsigned short *)malloc(EB_EFBLOCK_BUFFSIZE * WORDSIZE + sizeof(blocknum)); // blo
  unsigned int tmpbuf;
  set_ana();
  
  unsigned int readcnt = 0;
  unsigned int rp;     /* Read pointer */
  while(1){
    /* if online option is given */
    sem_p(semid, &semb);     // Lock shared memory
    memcpy((char *)&tblocknum, fshmp, sizeof(blocknum));
    if(blocknum != tblocknum){
      blocknum = tblocknum;
      memcpy((char *)&hd, shmp, sizeof(hd));
    }
    else{
      sleep(1);
      sem_v(semid, &semb);     // Unlock shared memory
      continue;
    }
    
    rhdl[0] = ridf_dechd(hd);
    layer = rhdl[0].layer;
    cid = rhdl[0].classid; 
    global_blksize = rhdl[0].blksize;
    #ifdef ONLINE_DEBUG
    cout << "---------------------------------------------" << endl;
    cout << "Global block header >> " << endl;
    cout << "   layer: " << layer << "  Class ID: " << cid << "  blksize: " << global_blksize << endl;
    #endif
    

    memcpy((char *)buf,shmp,(rhdl[0].blksize)*2);
    sem_v(semid, &semb);     // Unlock shared memory
    rp=HEADER_SIZE;

 
    
    /*****************************/
    while(rp<global_blksize){
      
      memcpy((char *)&hd,(char *)(buf+rp),sizeof(hd));
      rp+=HEADER_SIZE;
      
      rhdl[1]=ridf_dechd(hd);
      layer = rhdl[1].layer;
      cid = rhdl[1].classid; 
      blksize = rhdl[1].blksize;
      #ifdef ONLINE_DEBUG
      cout << "block header >> " << endl;
      cout << "  layer: " << layer << "  Class ID: " << cid << "  blksize: " << blksize << endl;
      #endif

      

    switch(cid){
    case RIDF_EF_BLOCK:  //Event Fragment
      break;
      
    case RIDF_EA_BLOCK:       // Event Assembly
      break;
      
    case RIDF_EAEF_BLOCK: // Event Assembly Fragment
      break;
      
    case RIDF_COMMENT:
      rp += blksize - HEADER_SIZE; // skip ridf comment
      // show_comment(blksize);
      break;
      
    case RIDF_END_BLOCK:       // end of block
      tmpbuf = ri(buf, &rp);
#ifdef ONLINE_DEBUG
      cout << ">> RIDF_END_NUMBER" << endl;
      cout << "   Number of this block: " << blk <<  endl;
#endif
      break;
      
    case RIDF_BLOCK_NUMBER: // Counter of the number of block
      tmpbuf = ri(buf, &rp);
      blk = tmpbuf;
#ifdef ONLINE_DEBUG
      cout << ">> RIDF_BLOCK_NUMBER" << endl;
      cout << "   Number of block: " << blk <<  endl;
#endif
      break;
      
      
    case RIDF_NCSCALER:
      //      printf("sca\n");
      rp += blksize - HEADER_SIZE; // skip cscaler
      while(readcnt<blksize){
        tmpbuf = ri(buf, &rp);
        readcnt+=2;
      }
      break;
      
    case RIDF_CSCALER:
      //      printf("csca\n");
      rp += blksize - HEADER_SIZE; // skip cscaler
      while(readcnt<blksize){
        tmpbuf = ri(buf, &rp);
      }
      break;
      
    case RIDF_NCSCALER32:
      //      printf("sca32\n");
      rp += blksize - HEADER_SIZE; // skip ncscaler32
      while(readcnt<blksize){
        // tmpbuf = ri(buf, &rp);
      }
      break;
      
    case RIDF_EVENT: // event data header
#ifdef ONLINE_DEBUG
      cout << ">> RIDF_EVENT" << endl;
#endif
      init_event();
      rp += dec_event_online(buf + rp, blksize);
      ana_event();
      tree->Fill();
      HistFill();
      break;
      
      case RIDF_SEGMENT: // segment data
      break;
      
      default:
      // printf("Error: RIDF Class ID:%d is invalid.\n",cid);
      break;
      if(STOP_FLAG==1){
	printf("bye-bye!\n");
	break;
      }
    
    }
    }
    // IMPORTANT: one should regularly call ProcessEvents
    // to let http server process requests
    if(gSystem->ProcessEvents()) break;
    
    if(STOP_FLAG==1){
      printf("bye-bye!\n");
      break;
    }
  } // end of while
  
  printf("eve: %d\n", eve);  
  printf("end of ana\n");
  
  
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

    rev =   (buf_header[0]>>30) & 0x3;
    layer = (buf_header[0]>>28) & 0x3;    
    cid = (buf_header[0]>>22) & 0x3f;    
    blksize= buf_header[0] & 0x003FFFFF;
    address = buf_header[1];
    //    printf("2. eve:%d, rev:%d, ly:%d, cid:%d, size:%d\n", eve, rev, layer, cid, blksize);

    switch(cid){
    case RIDF_SEGMENT:
      wrdcnt+=dec_segment(blksize);
      break;
    case RIDF_NCSCALER:
      printf("sca\n");
      break;
    case RIDF_CSCALER:
      printf("csca\n");
      break;
    case RIDF_NCSCALER32:
      printf("sca32\n");
      break;
    default:
      printf("Error: Invalid class ID: %d\n",cid);

      // skip this event
      while(wrdcnt<evesize){
        ridf.read((char*)buf_header, sizeof(buf_header));
        wrdcnt+=(sizeof(buf_header))*2;
      }
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

  case seg_mdpp16:
    dec_mdpp16(evt.mdpp16_hit_all, segsize, tmpdata);
    break;
    
  case seg_v792:
    break;

  case 4:
    break;
    
  default:
    printf("Unknown Module ID: %d\n", id.mod);
    break;
    
  } // end of switch(id.mod)
  
  return wrdcnt;
}

int analysis::dec_segment_online(unsigned int segsize, unsigned short *buf){
    unsigned int rps = 0;
    unsigned int segid;
    unsigned int tmpdata[MAXSIZE];


    segid = ri(buf, &rps);

    segmentID id;
    id.rev = (segid >> 26) & 0x3F;
    id.dev = (segid >> 20) & 0x3F;
    id.fp  = (segid >> 14) & 0x3F;
    id.det = (segid >>  8) & 0x3F;
    id.mod = segid & 0xFF;
    

    for(unsigned int i=0; rps < segsize; i++){
        tmpdata[i] = ri(buf, &rps);
    }


    switch(id.mod){
        case seg_v1190:
            ana_v1190(segsize, tmpdata, &evt);
            break;

        case seg_madc32:
            ana_mxdc32(evt.mxdc32_hit_all, segsize, tmpdata);
            break;

        case seg_mdpp16:
            dec_mdpp16(evt.mdpp16_hit_all, segsize, tmpdata);
            break;

        default:
            printf("Warning: Unknown Module ID: %d\n", id.mod);
            break;
    }

    return rps;
}

int analysis::dec_event_online(unsigned short *buf, unsigned int evesize){
  unsigned int rpi = 0;      

    eve = ri(buf, &rpi);

    printf(" Analyze: %d\r", eve);
    fflush(stdout);

    #ifdef ONLINE_DEBUG
    cout << " Event Number: " << eve << endl;
    cout << "evesize: " << evesize << endl;
    #endif
    
    // while(rpi < evesize * 2){
    while(rpi < evesize - HEADER_SIZE){ 
        
      for(int i=0; i<2; i++) buf_header[i] = ri(buf, &rpi); // Read 4 words

        rev     = (buf_header[0] >> 30) & 0x3;
        layer   = (buf_header[0] >> 28) & 0x3;    
        cid     = (buf_header[0] >> 22) & 0x3f;    
        blksize = buf_header[0] & 0x003FFFFF;
        address = buf_header[1];

#ifdef ONLINE_DEBUG
	cout << "Segment data >> " << endl;
	cout << "  layer: " << layer << "  Class ID: " << cid << "  blksize: " << blksize << endl;
	cout << "  rpi  : " << rpi << endl;
#endif
	
        switch(cid){
            case RIDF_SEGMENT:
	      rpi += dec_segment_online( blksize - HEADER_SIZE  , buf + rpi);
#ifdef ONLINE_DEBUG
	      cout << "  rpi  : " << rpi << endl;
#endif
	      break;

            case RIDF_NCSCALER:
            case RIDF_CSCALER:
            case RIDF_NCSCALER32:
                rpi += blksize - HEADER_SIZE;
                break;

            default:
                printf("Warning: Unknown class ID: %d\n", cid);
                rpi += blksize - HEADER_SIZE;
                break;
        }
    }
    
    return rpi;
}
