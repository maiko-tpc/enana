#include "analysis.hpp"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>
#include <sys/stat.h>
#include <TDirectory.h>
#include <TNamed.h>
#include <TParameter.h>

// Generic cut protocol, version g1, using the existing TNamed HTTP inbox.
//   g1|id|S|file.root|histName|dim|values...    Save cut
//   g1|id|L|file.root                         Load cut to editor
//   g1|id|A|histName|dim|values...            Apply edited cut
//   g1|id|F|file.root                         Apply saved cut
// 1D values = lower|upper
// 2D values = N|x1,y1|...|xN,yN (3 <= N <= 48).
// The server never evaluates code provided by the browser. Histogram names
// are checked against the compiled CutHistogramRegistry.
namespace {
struct GenericDefinition {
  std::string histogram;
  int dimension = 0;
  double lower = 0., upper = 0.;
  std::vector<std::pair<double,double>> points;
};

std::string decodeUrl(const std::string &source) {
  auto nibble = [](char c) -> int {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
  };
  std::string result;
  for (std::size_t i=0; i<source.size(); ++i) {
    if (source[i]=='%' && i+2<source.size()) {
      const int h=nibble(source[i+1]), l=nibble(source[i+2]);
      if (h>=0 && l>=0) {
        result += static_cast<char>((h<<4)|l);
        i+=2;
        continue;
      }
    }
    result += source[i];
  }
  return result;
}

std::vector<std::string> partsOf(const std::string &s) {
  std::istringstream stream(s);
  std::string part;
  std::vector<std::string> v;
  while (std::getline(stream, part, '|')) v.push_back(part);
  return v;
}

bool parseNumber(const std::string &s, double &x) {
  try {
    std::size_t used=0;
    if (s.empty()) return false;
    x=std::stod(s, &used);
    return used==s.size() && std::isfinite(x);
  } catch (...) { return false; }
}

bool fileNameOK(const std::string &name) {
  if (name.size()<6 || name.size()>69 ||
      name.compare(name.size()-5,5,".root")!=0 ||
      name.find("..")!=std::string::npos) return false;
  if (!((name.front()>='A'&&name.front()<='Z') ||
        (name.front()>='a'&&name.front()<='z') ||
        (name.front()>='0'&&name.front()<='9'))) return false;
  for (char c : name)
    if (!((c>='a'&&c<='z') || (c>='A'&&c<='Z') ||
          (c>='0'&&c<='9') || c=='_' || c=='-' || c=='.')) return false;
  return true;
}

bool regularFile(const std::string &path, bool required, std::string &error) {
  struct stat st;
  if (lstat(path.c_str(), &st)!=0) {
    if (!required && errno==ENOENT) return true;
    error="Cannot access file: " + path;
    return false;
  }
  if (!S_ISREG(st.st_mode)) {
    error="Refusing non-regular file: " + path;
    return false;
  }
  return true;
}

bool nonzeroPolygon(const std::vector<std::pair<double,double>> &v) {
  if (v.size()<3 || v.size()>48) return false;
  double minx=v[0].first,maxx=minx,miny=v[0].second,maxy=miny,area=0.;
  for (std::size_t i=0;i<v.size();++i) {
    const auto &a=v[i], &b=v[(i+1)%v.size()];
    if (!std::isfinite(a.first)||!std::isfinite(a.second)) return false;
    const double ax=a.first-v[0].first, ay=a.second-v[0].second;
    const double bx=b.first-v[0].first, by=b.second-v[0].second;
    area += ax*by-bx*ay;
    minx=std::min(minx,a.first);maxx=std::max(maxx,a.first);
    miny=std::min(miny,a.second);maxy=std::max(maxy,a.second);
  }
  const double scale=(maxx-minx)*(maxy-miny);
  return std::isfinite(scale) && scale>0 && std::abs(area)>scale*1e-10;
}

bool parseDefinition(const std::vector<std::string> &fields, std::size_t offset,
                     GenericDefinition &definition, std::string &error) {
  if (fields.size()<offset+3) {
    error="Incomplete cut definition";
    return false;
  }
  definition.histogram=fields[offset];
  if (fields[offset+1]=="1") {
    definition.dimension=1;
    if (fields.size()!=offset+4 ||
        !parseNumber(fields[offset+2],definition.lower) ||
        !parseNumber(fields[offset+3],definition.upper) ||
        !(definition.lower<definition.upper)) {
      error="Invalid 1D cut range";
      return false;
    }
  } else if (fields[offset+1]=="2") {
    definition.dimension=2;
    std::size_t count=0, pos=0;
    try {
      count=std::stoul(fields[offset+2], &pos, 10);
      if (pos!=fields[offset+2].size()) throw std::invalid_argument("count");
    } catch (...) { error="Invalid vertex count"; return false; }
    if (count<3 || count>48 || fields.size()!=offset+3+count) {
      error="Expected 3 to 48 vertices";
      return false;
    }
    for (std::size_t i=0;i<count;++i) {
      const std::string &pair=fields[offset+3+i];
      const auto comma=pair.find(',');
      if (comma==std::string::npos || pair.find(',',comma+1)!=std::string::npos) {
        error="Invalid vertex format";
        return false;
      }
      double x=0.,y=0.;
      if (!parseNumber(pair.substr(0,comma),x) ||
          !parseNumber(pair.substr(comma+1),y)) {
        error="Invalid vertex coordinates";
        return false;
      }
      definition.points.emplace_back(x,y);
    }
    if (!nonzeroPolygon(definition.points)) {
      error="Invalid polygon area";
      return false;
    }
  } else {
    error="Invalid histogram dimension";
    return false;
  }
  return true;
}

// Return a serialized definition without the leading status/id fields.
std::string serializeDefinition(const GenericDefinition &d) {
  std::ostringstream out;
  out << std::setprecision(17) << d.histogram << "|" << d.dimension;
  if (d.dimension==1) out << "|" << d.lower << "|" << d.upper;
  else {
    out << "|" << d.points.size();
    for (const auto &p:d.points) out << "|" << p.first << "," << p.second;
  }
  return out.str();
}

bool readCutFile(const std::string &filename, GenericDefinition &d,
                 std::string &error) {
  if (!fileNameOK(filename)) { error="Invalid .root file name"; return false; }
  const std::string path="macro/"+filename;
  if (!regularFile(path,true,error)) return false;
  TDirectory::TContext context;
  TFile file(path.c_str(), "READ");
  if (file.IsZombie()) { error="Cannot open ROOT file"; return false; }
  auto *hist=dynamic_cast<TNamed*>(file.Get("enana_cut_histogram"));
  auto *dim=dynamic_cast<TParameter<int>*>(file.Get("enana_cut_dimension"));
  if (hist && dim) {
    d.histogram=hist->GetTitle();
    d.dimension=dim->GetVal();
  } else if (!hist && !dim) {
    // Legacy F2 ROOT files contain only pidcut_f2.
    d.histogram="h_pid_f2";
    d.dimension=2;
  } else { error="Incomplete cut metadata"; return false; }
  if (d.dimension==1) {
    auto *lower=dynamic_cast<TParameter<double>*>(file.Get("enana_cut_lower"));
    auto *upper=dynamic_cast<TParameter<double>*>(file.Get("enana_cut_upper"));
    if (!lower || !upper || !std::isfinite(lower->GetVal()) ||
        !std::isfinite(upper->GetVal()) || lower->GetVal()>=upper->GetVal()) {
      error="Invalid 1D range in ROOT file";
      return false;
    }
    d.lower=lower->GetVal(); d.upper=upper->GetVal();
  } else if (d.dimension==2) {
    const char *key=(hist && dim && d.histogram!="h_pid_f2") ? "enana_cut2d" : "pidcut_f2";
    auto *cut=dynamic_cast<TCutG*>(file.Get(key));
    if (!cut) { error="No matching TCutG object"; return false; }
    const Int_t n=cut->GetN();
    if (n<3 || n>49) { error="Invalid saved vertex count"; return false; }
    for (Int_t i=0;i<n;++i) {
      double x=0.,y=0.;
      cut->GetPoint(i,x,y);
      d.points.emplace_back(x,y);
    }
    if (d.points.size()>3 &&
        d.points.front().first==d.points.back().first &&
        d.points.front().second==d.points.back().second) d.points.pop_back();
    if (!nonzeroPolygon(d.points)) { error="Invalid polygon in ROOT file"; return false; }
  } else { error="Unsupported cut dimension"; return false; }
  return true;
}

bool writeCutFile(const std::string &filename, const GenericDefinition &d,
                  const CutHistogramRegistry::Entry &entry,
                  std::string &error) {
  if (!fileNameOK(filename)) { error="Invalid .root file name"; return false; }
  if (filename=="pid_f2.root" &&
      (d.histogram!="h_pid_f2" || d.dimension!=2)) {
    error="pid_f2.root is reserved for the legacy F2 2D cut";
    return false;
  }
  const std::string path="macro/"+filename;
  const std::string tmp="macro/."+filename+".tmp";
  if (!regularFile(path,false,error) || !regularFile(tmp,false,error)) return false;
  struct stat st;
  if (lstat(path.c_str(),&st)==0) {
    GenericDefinition old;
    if (!readCutFile(filename,old,error)) {
      error="Refusing to replace unrelated ROOT file: "+error;
      return false;
    }
  }
  {
    TDirectory::TContext context;
    TFile output(tmp.c_str(),"RECREATE");
    if (output.IsZombie()) { error="Cannot create temporary ROOT file"; return false; }
    TNamed hist("enana_cut_histogram", d.histogram.c_str());
    TParameter<int> dim("enana_cut_dimension",d.dimension);
    output.cd();
    bool ok=hist.Write()>0 && dim.Write()>0;
    if (d.dimension==1) {
      TParameter<double> low("enana_cut_lower",d.lower);
      TParameter<double> high("enana_cut_upper",d.upper);
      ok=(low.Write()>0 && high.Write()>0 && ok);
    } else {
      // An unnamed constructor avoids ROOT's same-name TCutG replacement.
      TCutG cut;
      const char *key=d.histogram=="h_pid_f2" ? "pidcut_f2" : "enana_cut2d";
      cut.SetName(key);
      cut.SetVarX(entry.xExpression.c_str());
      cut.SetVarY(entry.yExpression.c_str());
      for (std::size_t i=0;i<d.points.size();++i)
        cut.SetPoint(static_cast<Int_t>(i),d.points[i].first,d.points[i].second);
      cut.SetPoint(static_cast<Int_t>(d.points.size()),
                   d.points[0].first,d.points[0].second);
      ok=(cut.Write(key)>0 && ok);
    }
    output.Close();
    if (!ok || output.TestBit(TFile::kWriteError)) {
      std::remove(tmp.c_str()); error="ROOT file write failed"; return false;
    }
  }
  if (std::rename(tmp.c_str(),path.c_str())!=0) {
    error="Cannot replace ROOT file: "+std::string(std::strerror(errno));
    std::remove(tmp.c_str());
    return false;
  }
  return true;
}
} // namespace

bool analysis::PassGenericF2Cut() const {
  if (!genericF2Cut.active) return false;
  double x=0.,y=0.;
  if (!cutHistogramRegistry.GetEventPoint(genericF2Cut.histogram,evt,x,y)) return false;
  if (genericF2Cut.dimension==1)
    return x>=genericF2Cut.lower && x<=genericF2Cut.upper;
  return genericF2Cut.polygon && genericF2Cut.polygon->IsInside(x,y);
}

void analysis::ProcessGenericCutRequest() {
  const std::string raw=genericCutRequest.GetTitle();
  if (raw.empty()) return;
  genericCutRequest.SetTitle("");
  const auto fields=partsOf(decodeUrl(raw));
  const std::string id=fields.size()>1 ? fields[1] : "unknown";
  auto fail=[&](const std::string &reason) {
    genericCutResult.SetTitle(("ERROR|"+id+"|"+reason).c_str());
    printf("Generic cut error: %s\n",reason.c_str());
  };
  if (raw.size()>16000 || fields.size()<4 || fields[0]!="g1" ||
      id.empty() || id.size()>32 ||
      !std::all_of(id.begin(),id.end(),[](unsigned char c){return c>='0'&&c<='9';})) {
    fail("Invalid cut request"); return;
  }
  const std::string action=fields[2];
  GenericDefinition definition;
  std::string error;
  std::string filename;
  if (action=="S" || action=="L" || action=="F") {
    if (fields.size()<4 || !fileNameOK(fields[3])) {
      fail("Invalid cut file name"); return;
    }
    filename=fields[3];
  }
  if (action=="S" || action=="A") {
    const std::size_t offset=action=="S"?4:3;
    if (!parseDefinition(fields,offset,definition,error)) { fail(error); return; }
  } else if (action=="L" || action=="F") {
    if (fields.size()!=4 || !readCutFile(filename,definition,error)) {
      fail(error.empty()?"Invalid load request":error); return;
    }
  } else { fail("Unsupported cut action"); return; }

  // Reject unregistered histograms and metadata mismatches on the server.
  const CutHistogramRegistry::Entry *entry=nullptr;
  for (const auto &e:cutHistogramRegistry.Entries())
    if (e.name==definition.histogram && e.dimension==definition.dimension) {
      entry=&e;
      break;
    }
  if (!entry) { fail("Histogram is not registered for this cut dimension"); return; }
  if (action=="S") {
    if (!writeCutFile(filename,definition,*entry,error)) { fail(error); return; }
    genericCutResult.SetTitle(("OK|"+id+"|Saved macro/"+filename).c_str());
    printf("Saved generic %dD cut for %s to macro/%s (not applied)\n",
           definition.dimension,definition.histogram.c_str(),filename.c_str());
    return;
  }
  if (action=="L") {
    genericCutResult.SetTitle(("OK|"+id+"|"+serializeDefinition(definition)).c_str());
    printf("Loaded generic cut from macro/%s (not applied)\n",filename.c_str());
    return;
  }

  // Prepare new cut completely BEFORE swapping it into the active state.
  std::unique_ptr<TCutG> polygon;
  if (definition.dimension==2) {
    polygon.reset(new TCutG());
    polygon->SetName("enana_generic_active_cut");
    polygon->SetVarX(entry->xExpression.c_str());
    polygon->SetVarY(entry->yExpression.c_str());
    for (std::size_t i=0;i<definition.points.size();++i)
      polygon->SetPoint(static_cast<Int_t>(i),definition.points[i].first,
                        definition.points[i].second);
    polygon->SetPoint(static_cast<Int_t>(definition.points.size()),
                      definition.points[0].first,definition.points[0].second);
  }
  genericF2Cut.polygon.swap(polygon);
  genericF2Cut.histogram=definition.histogram;
  genericF2Cut.dimension=definition.dimension;
  genericF2Cut.lower=definition.lower;
  genericF2Cut.upper=definition.upper;
  genericF2Cut.active=true;
  evt.use_pid_f2=1;
  const std::string source=action=="F"?"macro/"+filename:"edited selection";
  genericCutResult.SetTitle(("OK|"+id+"|Applied "+source).c_str());
  printf("Applied generic %dD cut on %s to is_pid_f2: subsequent events only\n",
         definition.dimension,definition.histogram.c_str());
}
