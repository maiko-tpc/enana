#include "analysis.hpp"
#include "ana_event.hpp"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <TDirectory.h>
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

  // Clone cuts before closing their ROOT files. Save Cut may replace the F2
  // file while the previous cut remains active in this process.
  pid_f2 = nullptr;
  pid_f3 = nullptr;
  {
    TFile file_pid_f2("macro/pid_f2.root", "READ");
    if (!file_pid_f2.IsZombie()) {
      auto *stored = dynamic_cast<TCutG*>(file_pid_f2.Get("pidcut_f2"));
      if (stored) pid_f2 = static_cast<TCutG*>(stored->Clone("pidcut_f2"));
    }
  }
  if (pid_f2 && pid_f2->GetN() > 2) evt.use_pid_f2 = 1;

  {
    TFile file_pid_f3("macro/pid_f3.root", "READ");
    if (!file_pid_f3.IsZombie()) {
      auto *stored = dynamic_cast<TCutG*>(file_pid_f3.Get("pidcut_f3"));
      if (stored) pid_f3 = static_cast<TCutG*>(stored->Clone("pidcut_f3"));
    }
  }
  if (pid_f3 && pid_f3->GetN() > 2) evt.use_pid_f3 = 1;

}

analysis::~analysis(){
}

// Load Cut request wire format: a decimal request ID set in cut_load_request.
// Loads the *saved* TCutG; it never replaces pid_f2 used by analysis.
// Response: OK|request-id|N|x1,y1|...|xN,yN (without a repeated closing vertex)
void analysis::ProcessCutLoadRequest(){
  const std::string id = cutLoadRequest.GetTitle();
  if (id.empty()) return;
  cutLoadRequest.SetTitle("");

  auto fail = [&](const std::string &reason) {
    cutLoadResult.SetTitle(("ERROR|" + id + "|" + reason).c_str());
    printf("Cut load failed: %s\n", reason.c_str());
  };

  if (id.size() > 32 ||
      !std::all_of(id.begin(), id.end(), [](unsigned char c) {
        return c >= '0' && c <= '9';
      })) {
    fail("Invalid request ID");
    return;
  }

  TDirectory::TContext restoreDirectory;
  TFile input("macro/pid_f2.root", "READ");
  if (input.IsZombie()) {
    fail("Cannot open macro/pid_f2.root");
    return;
  }
  auto *cut = dynamic_cast<TCutG*>(input.Get("pidcut_f2"));
  if (!cut) {
    fail("pidcut_f2 was not found in macro/pid_f2.root");
    return;
  }

  int n = cut->GetN();
  if (n < 3 || n > 49) {
    fail("Saved cut must contain 3 to 48 vertices");
    return;
  }

  // Save Cut writes a polygon with its first vertex repeated at the end.
  double first_x = 0.0, first_y = 0.0;
  double last_x = 0.0, last_y = 0.0;
  cut->GetPoint(0, first_x, first_y);
  cut->GetPoint(n-1, last_x, last_y);
  if (n >= 4 && first_x == last_x && first_y == last_y) --n;
  if (n < 3 || n > 48) {
    fail("Saved cut must contain 3 to 48 unique vertices");
    return;
  }

  std::ostringstream response;
  response << "OK|" << id << "|" << n << std::setprecision(17);
  for (int i=0; i<n; ++i) {
    double x = 0.0, y = 0.0;
    cut->GetPoint(i, x, y);
    if (!std::isfinite(x) || !std::isfinite(y)) {
      fail("Saved cut has invalid coordinates");
      return;
    }
    response << "|" << x << "," << y;
  }
  cutLoadResult.SetTitle(response.str().c_str());
  printf("Loaded saved Web TCutG pidcut_f2 (%d vertices) from macro/pid_f2.root (not applied)\n", n);
}

// Save request wire format (URL encoded as SetTitle argument):
//   v1|request-id|N|x1,y1|x2,y2|...|xN,yN
// The editor only saves F2; no code is executed from user-supplied text.
void analysis::ProcessCutSaveRequest(){
  // THttpServer may leave URL percent-encoding in the SetTitle() argument.
  // Decode only well-formed %XX sequences. If already decoded, keep it intact.
  auto decodePercent = [](const std::string &encoded) {
    auto hex = [](char ch) -> int {
      if (ch >= '0' && ch <= '9') return ch - '0';
      if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
      if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
      return -1;
    };
    std::string decoded;
    decoded.reserve(encoded.size());
    for (std::size_t i = 0; i < encoded.size(); ++i) {
      if (encoded[i] == '%' && i + 2 < encoded.size()) {
        const int hi = hex(encoded[i+1]);
        const int lo = hex(encoded[i+2]);
        if (hi >= 0 && lo >= 0) {
          decoded.push_back(static_cast<char>((hi << 4) | lo));
          i += 2;
          continue;
        }
      }
      decoded.push_back(encoded[i]);
    }
    return decoded;
  };

  const std::string encoded = cutSaveRequest.GetTitle();
  if (encoded.empty()) return;
  cutSaveRequest.SetTitle("");
  const std::string payload = decodePercent(encoded);

  std::vector<std::string> fields;
  std::istringstream input(payload);
  std::string field;
  while (std::getline(input, field, '|')) fields.push_back(field);
  if (fields.size() < 3) {
    cutSaveResult.SetTitle("ERROR|unknown|Malformed cut request");
    printf("Cut save failed: Malformed cut request (%zu fields, %zu encoded bytes)\n",
           fields.size(), encoded.size());
    return;
  }
  const std::string id = fields[1];
  auto fail = [&](const std::string &reason) {
    cutSaveResult.SetTitle(("ERROR|" + id + "|" + reason).c_str());
    printf("Cut save failed: %s\n", reason.c_str());
  };
  if (fields[0] != "v1" || id.empty() || id.size() > 32 ||
      !std::all_of(id.begin(), id.end(), [](unsigned char c){ return c >= '0' && c <= '9'; })) {
    fail("Invalid request ID or protocol");
    return;
  }
  std::size_t n = 0;
  try {
    std::size_t pos = 0;
    n = std::stoul(fields[2], &pos, 10);
    if (pos != fields[2].size()) throw std::invalid_argument("not an integer");
  } catch (...) {
    fail("Invalid vertex count");
    return;
  }
  if (n < 3 || n > 48 || fields.size() != n + 3) {
    fail("Expected 3 to 48 vertices");
    return;
  }

  std::vector<std::pair<double,double>> vertices;
  vertices.reserve(n);
  for (std::size_t i = 0; i < n; ++i) {
    const std::string &s = fields[i+3];
    const auto comma = s.find(',');
    if (comma == std::string::npos || s.find(',', comma+1) != std::string::npos) {
      fail("Invalid vertex format");
      return;
    }
    try {
      std::size_t px = 0, py = 0;
      double x = std::stod(s.substr(0,comma), &px);
      double y = std::stod(s.substr(comma+1), &py);
      if (px != comma || py != s.size()-comma-1 ||
          !std::isfinite(x) || !std::isfinite(y)) throw std::invalid_argument("invalid coordinate");
      vertices.emplace_back(x,y);
    } catch (...) {
      fail("Invalid numeric coordinates");
      return;
    }
  }
  // Area check with coordinates translated to first vertex to reduce
  // floating-point cancellation for axes far from zero.
  double twice_area = 0.;
  double xmin = vertices[0].first, xmax = xmin;
  double ymin = vertices[0].second, ymax = ymin;
  for (std::size_t i=0; i<n; ++i) {
    const auto &a = vertices[i];
    const auto &b = vertices[(i+1)%n];
    double ax = a.first - vertices[0].first;
    double ay = a.second - vertices[0].second;
    double bx = b.first - vertices[0].first;
    double by = b.second - vertices[0].second;
    twice_area += ax*by-bx*ay;
    xmin = std::min(xmin,a.first); xmax = std::max(xmax,a.first);
    ymin = std::min(ymin,a.second); ymax = std::max(ymax,a.second);
  }
  const double span_area = (xmax-xmin)*(ymax-ymin);
  if (!std::isfinite(twice_area) || !std::isfinite(span_area) ||
      span_area <= 0 || std::abs(twice_area) <= span_area*1.e-10) {
    fail("Degenerate polygon");
    return;
  }

  const char *tmp_name = "macro/pid_f2.root.tmp";
  const char *final_name = "macro/pid_f2.root";
  {
    TDirectory::TContext restoreDirectory;
    TFile output(tmp_name, "RECREATE");
    if (output.IsZombie()) {
      fail("Cannot open temporary ROOT file");
      return;
    }
    TCutG cut("pidcut_f2", static_cast<Int_t>(n+1));
    cut.SetVarX("rf[2]-ref_tdc");
    cut.SetVarY("ssd.ene[0]");
    for (std::size_t i=0; i<n; ++i)
      cut.SetPoint(static_cast<Int_t>(i),vertices[i].first,vertices[i].second);
    cut.SetPoint(static_cast<Int_t>(n),vertices[0].first,vertices[0].second);
    output.cd();
    const auto written = cut.Write("pidcut_f2", TObject::kOverwrite);
    output.Close();
    if (written <= 0 || output.TestBit(TFile::kWriteError)) {
      std::remove(tmp_name);
      fail("ROOT file write failed");
      return;
    }
  }
  if (std::rename(tmp_name, final_name) != 0) {
    const std::string reason = std::strerror(errno);
    std::remove(tmp_name);
    fail("Could not replace cut file: " + reason);
    return;
  }
  cutSaveResult.SetTitle(("OK|" + id + "|Saved to macro/pid_f2.root").c_str());
  printf("Saved Web TCutG pidcut_f2 (%zu vertices) to %s (not applied to running analysis)\n",
         n, final_name);
}

void analysis::SetPID(int pidnum){
  pid = pidnum;
}

void analysis::set_ana(){

  // CH mapping of TDC
  int ppac_geo = 7;
  //  int ppac_geo = 0;  
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
    bool exit_requested = gSystem->ProcessEvents();
    ProcessCutSaveRequest();
    ProcessCutLoadRequest();
    if(exit_requested) break;
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
      sem_v(semid, &semb);     // Unlock shared memory before HTTP handling
      bool exit_requested = gSystem->ProcessEvents();
      ProcessCutSaveRequest();
      ProcessCutLoadRequest();
      if (exit_requested || STOP_FLAG==1) break;
      usleep(100000);
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
    bool exit_requested = gSystem->ProcessEvents();
    ProcessCutSaveRequest();
    ProcessCutLoadRequest();
    if(exit_requested) break;

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
  // RIDF block sizes are measured in 16-bit words.
  constexpr unsigned int RIDF_WORD_SIZE = 2; // bytes in one RIDF word
  const unsigned int header_words = sizeof(buf_header) / RIDF_WORD_SIZE;
  
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
    wrdcnt += header_words;

    rev =   (buf_header[0]>>30) & 0x3;
    layer = (buf_header[0]>>28) & 0x3;    
    cid = (buf_header[0]>>22) & 0x3f;    
    blksize= buf_header[0] & 0x003FFFFF;
    address = buf_header[1];
    //    printf("2. eve:%d, rev:%d, ly:%d, cid:%d, size:%d\n", eve, rev, layer, cid, blksize);

    switch(cid){
    case RIDF_SEGMENT:
      // dec_segment() returns the whole segment size, including its
      // 4-word RIDF header, which was already counted above.
      wrdcnt += dec_segment(blksize) - header_words;
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

  case seg_v830:
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
