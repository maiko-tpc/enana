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
#include <dirent.h>
#include <sys/stat.h>
// #define ONLINE_DEBUG

// F2 cut files are intentionally restricted to plain .root filenames in macro/.
namespace {
std::string cutPercentDecode(const std::string &encoded) {
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
}

bool validCutRequestId(const std::string &id) {
  return !id.empty() && id.size() <= 32 &&
         std::all_of(id.begin(), id.end(), [](unsigned char c) {
           return c >= '0' && c <= '9';
         });
}

bool validCutFileName(const std::string &name) {
  if (name.size() < 6 || name.size() > 69 ||
      name.compare(name.size() - 5, 5, ".root") != 0 ||
      name.find("..") != std::string::npos) return false;
  if (!((name[0] >= 'A' && name[0] <= 'Z') ||
        (name[0] >= 'a' && name[0] <= 'z') ||
        (name[0] >= '0' && name[0] <= '9'))) return false;
  for (char c : name) {
    if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
          (c >= '0' && c <= '9') || c == '_' || c == '.' || c == '-'))
      return false;
  }
  return true;
}

std::string cutFilePath(const std::string &name) {
  return "macro/" + name;
}

// Do not follow symbolic links out of the macro/ directory.
bool checkCutFileType(const std::string &path, bool mustExist,
                      std::string &error) {
  struct stat info;
  if (lstat(path.c_str(), &info) != 0) {
    if (!mustExist && errno == ENOENT) return true;
    error = "Cannot access " + path + ": " + std::strerror(errno);
    return false;
  }
  if (!S_ISREG(info.st_mode)) {
    error = "Cut file is not a regular file: " + path;
    return false;
  }
  return true;
}

bool cutPolygonValid(const std::vector<std::pair<double,double>> &vertices) {
  const std::size_t n = vertices.size();
  if (n < 3 || n > 48) return false;
  double twice_area = 0.;
  double xmin = vertices[0].first, xmax = xmin;
  double ymin = vertices[0].second, ymax = ymin;
  for (std::size_t i = 0; i < n; ++i) {
    const auto &a = vertices[i];
    const auto &b = vertices[(i+1)%n];
    if (!std::isfinite(a.first) || !std::isfinite(a.second)) return false;
    const double ax = a.first - vertices[0].first;
    const double ay = a.second - vertices[0].second;
    const double bx = b.first - vertices[0].first;
    const double by = b.second - vertices[0].second;
    twice_area += ax*by - bx*ay;
    xmin = std::min(xmin, a.first); xmax = std::max(xmax, a.first);
    ymin = std::min(ymin, a.second); ymax = std::max(ymax, a.second);
  }
  const double span_area = (xmax-xmin)*(ymax-ymin);
  return std::isfinite(twice_area) && std::isfinite(span_area) &&
         span_area > 0 && std::abs(twice_area) > span_area*1.e-10;
}

bool loadF2CutVertices(const std::string &name,
                       std::vector<std::pair<double,double>> &vertices,
                       std::string &error) {
  const std::string path = cutFilePath(name);
  if (!checkCutFileType(path, true, error)) return false;
  TDirectory::TContext restoreDirectory;
  TFile input(path.c_str(), "READ");
  if (input.IsZombie()) {
    error = "Cannot open " + path;
    return false;
  }
  auto *cut = dynamic_cast<TCutG*>(input.Get("pidcut_f2"));
  if (!cut) {
    error = "pidcut_f2 was not found in " + path;
    return false;
  }
  int n = cut->GetN();
  if (n < 3 || n > 49) {
    error = "Saved cut must contain 3 to 48 vertices";
    return false;
  }
  double fx = 0., fy = 0., lx = 0., ly = 0.;
  cut->GetPoint(0, fx, fy);
  cut->GetPoint(n-1, lx, ly);
  if (n >= 4 && fx == lx && fy == ly) --n;
  if (n < 3 || n > 48) {
    error = "Saved cut must contain 3 to 48 unique vertices";
    return false;
  }
  vertices.clear();
  vertices.reserve(n);
  for (int i=0; i<n; ++i) {
    double x = 0., y = 0.;
    cut->GetPoint(i, x, y);
    vertices.emplace_back(x,y);
  }
  if (!cutPolygonValid(vertices)) {
    error = "Saved cut contains invalid or degenerate coordinates";
    return false;
  }
  return true;
}

bool parseEditorPolygon(const std::vector<std::string> &fields,
                        std::size_t countField, 
                        std::vector<std::pair<double,double>> &vertices,
                        std::string &error) {
  std::size_t n = 0;
  if (fields.size() <= countField) {
    error = "Missing vertex count";
    return false;
  }
  try {
    std::size_t pos = 0;
    n = std::stoul(fields[countField], &pos, 10);
    if (pos != fields[countField].size()) throw std::invalid_argument("not integer");
  } catch (...) {
    error = "Invalid vertex count";
    return false;
  }
  if (n < 3 || n > 48 || fields.size() != n + countField + 1) {
    error = "Expected 3 to 48 vertices";
    return false;
  }
  vertices.clear();
  vertices.reserve(n);
  for (std::size_t i=0; i<n; ++i) {
    const std::string &s = fields[i+countField+1];
    const auto comma = s.find(',');
    if (comma == std::string::npos || s.find(',', comma+1) != std::string::npos) {
      error = "Invalid vertex format";
      return false;
    }
    try {
      std::size_t px = 0, py = 0;
      const double x = std::stod(s.substr(0,comma), &px);
      const double y = std::stod(s.substr(comma+1), &py);
      if (px != comma || py != s.size()-comma-1 ||
          !std::isfinite(x) || !std::isfinite(y))
        throw std::invalid_argument("invalid numeric coordinate");
      vertices.emplace_back(x,y);
    } catch (...) {
      error = "Invalid numeric coordinates";
      return false;
    }
  }
  if (!cutPolygonValid(vertices)) {
    error = "Degenerate polygon";
    return false;
  }
  return true;
}
}


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
// Load Cut accepts either the legacy numeric request ID (default file) or
// v2|request-id|filename.root. Returns editor coordinates without applying.
void analysis::ProcessCutLoadRequest(){
  const std::string raw = cutLoadRequest.GetTitle();
  if (raw.empty()) return;
  cutLoadRequest.SetTitle("");
  const std::string payload = cutPercentDecode(raw);
  std::string id = payload, name = "pid_f2.root";
  if (payload.compare(0, 3, "v2|") == 0) {
    std::vector<std::string> fields;
    std::istringstream in(payload);
    std::string field;
    while (std::getline(in, field, '|')) fields.push_back(field);
    if (fields.size() != 3) {
      cutLoadResult.SetTitle("ERROR|unknown|Malformed load request");
      return;
    }
    id = fields[1];
    name = fields[2];
  }
  auto fail = [&](const std::string &reason) {
    cutLoadResult.SetTitle(("ERROR|" + id + "|" + reason).c_str());
    printf("Cut load failed: %s\n", reason.c_str());
  };
  if (!validCutRequestId(id)) {
    fail("Invalid request ID");
    return;
  }
  if (!validCutFileName(name)) {
    fail("Invalid cut filename (use a .root basename in macro/)");
    return;
  }
  std::vector<std::pair<double,double>> vertices;
  std::string error;
  if (!loadF2CutVertices(name, vertices, error)) {
    fail(error);
    return;
  }
  std::ostringstream response;
  response << "OK|" << id << "|" << vertices.size() << std::setprecision(17);
  for (const auto &p : vertices) response << "|" << p.first << "," << p.second;
  cutLoadResult.SetTitle(response.str().c_str());
  printf("Loaded saved Web TCutG pidcut_f2 (%zu vertices) from %s (not applied)\n",
         vertices.size(), cutFilePath(name).c_str());
}

// Save request wire format (URL encoded as SetTitle argument):
//   v1|request-id|N|x1,y1|x2,y2|...|xN,yN
// The editor only saves F2; no code is executed from user-supplied text.
// Save Cut accepts legacy v1|id|N|x,y... (default file) and
// v2|id|filename.root|N|x,y... (selected file).
void analysis::ProcessCutSaveRequest(){
  const std::string raw = cutSaveRequest.GetTitle();
  if (raw.empty()) return;
  cutSaveRequest.SetTitle("");
  const std::string payload = cutPercentDecode(raw);
  std::vector<std::string> fields;
  std::istringstream in(payload);
  std::string field;
  while (std::getline(in, field, '|')) fields.push_back(field);
  if (fields.size() < 3) {
    cutSaveResult.SetTitle("ERROR|unknown|Malformed cut request");
    return;
  }
  const std::string id = fields[1];
  auto fail = [&](const std::string &reason) {
    cutSaveResult.SetTitle(("ERROR|" + id + "|" + reason).c_str());
    printf("Cut save failed: %s\n", reason.c_str());
  };
  const bool v1 = (fields[0] == "v1");
  const bool v2 = (fields[0] == "v2");
  if ((!v1 && !v2) || !validCutRequestId(id)) {
    fail("Invalid request ID or protocol");
    return;
  }
  const std::size_t countField = v2 ? 3 : 2;
  const std::string name = v2 && fields.size() > 2 ? fields[2] : "pid_f2.root";
  if (!validCutFileName(name)) {
    fail("Invalid cut filename (use a .root basename in macro/)");
    return;
  }
  std::vector<std::pair<double,double>> vertices;
  std::string error;
  if (!parseEditorPolygon(fields, countField, vertices, error)) {
    fail(error);
    return;
  }
  const std::string finalName = cutFilePath(name);
  const std::string tmpName = cutFilePath("." + name + ".tmp");
  if (!checkCutFileType(finalName, false, error) ||
      !checkCutFileType(tmpName, false, error)) {
    fail(error);
    return;
  }
  // Never overwrite unrelated ROOT files (e.g. macro/pid_f3.root).
  struct stat existingInfo;
  if (lstat(finalName.c_str(), &existingInfo) == 0) {
    std::vector<std::pair<double,double>> existingCut;
    if (!loadF2CutVertices(name, existingCut, error)) {
      fail("Refusing to overwrite an existing file without a valid pidcut_f2: " + error);
      return;
    }
  }
  {
    TDirectory::TContext restoreDirectory;
    TFile output(tmpName.c_str(), "RECREATE");
    if (output.IsZombie()) {
      fail("Cannot open temporary ROOT file");
      return;
    }
    // Never use the named TCutG constructor: ROOT can delete a same-named cut
    // from its specials list while that cut is still active in analysis.
    TCutG cut;
    cut.SetName("pidcut_f2");
    cut.SetVarX("rf[2]-ref_tdc");
    cut.SetVarY("ssd.ene[0]");
    for (std::size_t i=0; i<vertices.size(); ++i)
      cut.SetPoint(static_cast<Int_t>(i), vertices[i].first, vertices[i].second);
    cut.SetPoint(static_cast<Int_t>(vertices.size()),
                 vertices[0].first, vertices[0].second);
    output.cd();
    const auto written = cut.Write("pidcut_f2", TObject::kOverwrite);
    output.Close();
    if (written <= 0 || output.TestBit(TFile::kWriteError)) {
      std::remove(tmpName.c_str());
      fail("ROOT file write failed");
      return;
    }
  }
  if (std::rename(tmpName.c_str(), finalName.c_str()) != 0) {
    const std::string reason = std::strerror(errno);
    std::remove(tmpName.c_str());
    fail("Could not replace cut file: " + reason);
    return;
  }
  cutSaveResult.SetTitle(("OK|" + id + "|Saved to " + finalName).c_str());
  printf("Saved Web TCutG pidcut_f2 (%zu vertices) to %s (not applied to running analysis)\n",
         vertices.size(), finalName.c_str());
}

// Apply Cut: replaces only the in-memory F2 PID cut; does not change the ROOT file.
// Already-filled events, histograms and tree entries are unchanged.
// Apply Cut accepts an edited polygon (v1|id|N|x,y...) OR a selected saved
// file (file2|id|filename.root). Neither operation writes the ROOT cut file.
void analysis::ProcessCutApplyRequest(){
  const std::string raw = cutApplyRequest.GetTitle();
  if (raw.empty()) return;
  cutApplyRequest.SetTitle("");
  const std::string payload = cutPercentDecode(raw);
  std::vector<std::string> fields;
  std::istringstream in(payload);
  std::string field;
  while (std::getline(in, field, '|')) fields.push_back(field);
  if (fields.size() < 3) {
    cutApplyResult.SetTitle("ERROR|unknown|Malformed cut request");
    return;
  }
  const std::string id = fields[1];
  auto fail = [&](const std::string &reason) {
    cutApplyResult.SetTitle(("ERROR|" + id + "|" + reason).c_str());
    printf("Cut apply failed: %s\n", reason.c_str());
  };
  if (!validCutRequestId(id)) {
    fail("Invalid request ID");
    return;
  }
  std::vector<std::pair<double,double>> vertices;
  std::string error, source;
  if (fields[0] == "file2") {
    if (fields.size() != 3 || !validCutFileName(fields[2])) {
      fail("Invalid cut filename (use a .root basename in macro/)");
      return;
    }
    source = cutFilePath(fields[2]);
    if (!loadF2CutVertices(fields[2], vertices, error)) {
      fail(error);
      return;
    }
  } else if (fields[0] == "v1") {
    if (!parseEditorPolygon(fields, 2, vertices, error)) {
      fail(error);
      return;
    }
    source = "edited polygon";
  } else {
    fail("Invalid protocol");
    return;
  }
  // Construct unnamed, then SetName: ROOT's named TCutG constructor may
  // delete an existing same-name cut and leave pid_f2 dangling.
  auto *next = new TCutG();
  next->SetName("pidcut_f2");
  next->SetVarX("rf[2]-ref_tdc");
  next->SetVarY("ssd.ene[0]");
  for (std::size_t i=0; i<vertices.size(); ++i)
    next->SetPoint(static_cast<Int_t>(i), vertices[i].first, vertices[i].second);
  next->SetPoint(static_cast<Int_t>(vertices.size()),
                 vertices[0].first, vertices[0].second);
  TCutG *previous = pid_f2;
  pid_f2 = next;
  genericF2Cut.active = false; // Legacy F2 Apply overrides generic selection
  evt.use_pid_f2 = 1;
  delete previous;
  cutApplyResult.SetTitle(("OK|" + id + "|Applied " + source).c_str());
  printf("Applied Web TCutG pidcut_f2 (%zu vertices) from %s: subsequent events only\n",
         vertices.size(), source.c_str());
}


// List existing .root filenames under macro/ for the browser's picker.
// Only plain regular files are exposed; the protocol never accepts a path.
void analysis::ProcessCutListRequest(){
  const std::string id = cutListRequest.GetTitle();
  if (id.empty()) return;
  cutListRequest.SetTitle("");
  if (!validCutRequestId(id)) {
    cutListResult.SetTitle("ERROR|unknown|Invalid request ID");
    return;
  }
  DIR *dir = opendir("macro");
  if (!dir) {
    cutListResult.SetTitle(("ERROR|" + id + "|Cannot open macro directory").c_str());
    return;
  }
  std::vector<std::string> files;
  while (auto *entry = readdir(dir)) {
    const std::string name(entry->d_name);
    if (!validCutFileName(name)) continue;
    std::string error;
    if (checkCutFileType(cutFilePath(name), true, error))
      files.push_back(name);
  }
  closedir(dir);
  std::sort(files.begin(), files.end());
  std::ostringstream response;
  response << "OK|" << id;
  for (const auto &name : files) response << "|" << name;
  cutListResult.SetTitle(response.str().c_str());
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
    ProcessCutListRequest();
    ProcessCutApplyRequest();
    ProcessGenericCutRequest();
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
    ProcessCutListRequest();
      ProcessCutApplyRequest();
      ProcessGenericCutRequest();
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
    ProcessCutListRequest();
    ProcessCutApplyRequest();
    ProcessGenericCutRequest();
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
