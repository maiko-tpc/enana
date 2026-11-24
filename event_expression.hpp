#ifndef ENANA_EVENT_EXPRESSION_HPP
#define ENANA_EVENT_EXPRESSION_HPP

// Tiny, restricted expression compiler for one-point-per-event histograms.
// Expressions are compiled ONCE at histogram booking, not interpreted for
// every event. Only explicitly listed evtdata members are accessible.
// This header does not use ROOT reflection, Cling or runtime C++ execution.

#include "evtdata.hpp"
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace enana_expression {
using Value = std::function<double(const evtdata&)>;
using Valid = std::function<bool(const evtdata&)>;

inline bool MatchArray(const std::string &name, const char *prefix,
                       const std::vector<int> &bounds, std::vector<int> &indices) {
  const std::string p(prefix);
  if (name.compare(0, p.size(), p) != 0) return false;
  indices.clear();
  std::size_t pos = p.size();
  for (int bound : bounds) {
    if (pos >= name.size() || name[pos++] != '[')
      throw std::invalid_argument("Expected array index in: " + name);
    if (pos >= name.size() || !std::isdigit(static_cast<unsigned char>(name[pos])))
      throw std::invalid_argument("Invalid array index in: " + name);
    int index = 0;
    while (pos < name.size() && std::isdigit(static_cast<unsigned char>(name[pos]))) {
      int digit = name[pos++] - '0';
      if (index > bound) throw std::invalid_argument("Array index out of range in: " + name);
      index = index * 10 + digit;
      if (index >= bound) throw std::invalid_argument("Array index out of range in: " + name);
    }
    if (pos >= name.size() || name[pos++] != ']')
      throw std::invalid_argument("Missing ']' in: " + name);
    indices.push_back(index);
  }
  if (pos != name.size())
    throw std::invalid_argument("Unexpected array suffix in: " + name);
  return true;
}

inline Value ResolveVariable(const std::string &s) {
  if (s == "ref_tdc") return [](const evtdata &e) { return double(e.ref_tdc); };
  if (s == "eve") return [](const evtdata &e) { return double(e.eve); };
  if (s == "is_pid_f2") return [](const evtdata &e) { return double(e.is_pid_f2); };
  if (s == "is_pid_f3") return [](const evtdata &e) { return double(e.is_pid_f3); };
  std::vector<int> i;
  if (MatchArray(s, "rf", {N_RF}, i)) { int a=i[0]; return [a](const evtdata &e){return double(e.rf[a]);}; }
  if (MatchArray(s, "ssd.ene", {N_SSD}, i)) { int a=i[0]; return [a](const evtdata &e){return double(e.ssd.ene[a]);}; }
  if (MatchArray(s, "ssd.adc", {N_SSD}, i)) { int a=i[0]; return [a](const evtdata &e){return double(e.ssd.adc[a]);}; }
  if (MatchArray(s, "ssd.tdc", {N_SSD}, i)) { int a=i[0]; return [a](const evtdata &e){return double(e.ssd.tdc[a]);}; }
  if (MatchArray(s, "ssd_ene", {N_SSD}, i)) { int a=i[0]; return [a](const evtdata &e){return double(e.ssd_ene[a]);}; }
  if (MatchArray(s, "madc.adc", {N_MADC_CH}, i)) { int a=i[0]; return [a](const evtdata &e){return double(e.madc.adc[a]);}; }
  if (MatchArray(s, "mdpp.adc", {N_MDPP_CH}, i)) { int a=i[0]; return [a](const evtdata &e){return double(e.mdpp.adc[a]);}; }
  if (MatchArray(s, "ppac_good", {N_PPAC}, i)) { int a=i[0]; return [a](const evtdata &e){return double(e.ppac_good[a]);}; }
  if (MatchArray(s, "ppac_tdc", {N_PPAC,N_PPAC_CH}, i)) { int a=i[0],b=i[1]; return [a,b](const evtdata &e){return double(e.ppac_tdc[a][b]);}; }
  if (MatchArray(s, "ppac_pos_raw", {N_PPAC,2}, i)) { int a=i[0],b=i[1]; return [a,b](const evtdata &e){return double(e.ppac_pos_raw[a][b]);}; }
  if (MatchArray(s, "ppac_pos_cal", {N_PPAC,2}, i)) { int a=i[0],b=i[1]; return [a,b](const evtdata &e){return double(e.ppac_pos_cal[a][b]);}; }
  if (MatchArray(s, "ppac_ang", {2,2}, i)) { int a=i[0],b=i[1]; return [a,b](const evtdata &e){return double(e.ppac_ang[a][b]);}; }
  if (MatchArray(s, "f2_pos", {2}, i)) { int a=i[0]; return [a](const evtdata &e){return double(e.f2_pos[a]);}; }
  if (MatchArray(s, "pl_tdc", {N_PL_CH}, i)) { int a=i[0]; return [a](const evtdata &e){return double(e.pl_tdc[a]);}; }
  if (MatchArray(s, "tdc_lead", {128}, i)) { int a=i[0]; return [a](const evtdata &e){return double(e.tdc_lead[a]);}; }
  if (MatchArray(s, "tdc_hit", {128}, i)) { int a=i[0]; return [a](const evtdata &e){return double(e.tdc_hit[a]);}; }
  if (MatchArray(s, "tdc_multi", {128}, i)) { int a=i[0]; return [a](const evtdata &e){return double(e.tdc_multi[a]);}; }
  throw std::invalid_argument("Unsupported event variable: " + s);
}

class Parser {
public:
  explicit Parser(std::string source) : source_(std::move(source)) {
    if (source_.empty() || source_.size() > 256)
      throw std::invalid_argument("Empty or oversized event expression");
  }
  Value Compile() {
    Value result = ParseOr();
    Space();
    if (pos_ != source_.size())
      throw std::invalid_argument("Unexpected token in event expression: " + source_.substr(pos_));
    return result;
  }
private:
  std::string source_;
  std::size_t pos_ = 0;
  int nodes_ = 0;
  void Node() {
    if (++nodes_ > 128) throw std::invalid_argument("Event expression too complex");
  }
  void Space() {
    while (pos_ < source_.size() && std::isspace(static_cast<unsigned char>(source_[pos_]))) ++pos_;
  }
  bool Eat(const char *token) {
    Space();
    const std::string t(token);
    if (source_.compare(pos_, t.size(), t) != 0) return false;
    pos_ += t.size();
    return true;
  }
  Value ParseOr() {
    Value left = ParseAnd();
    while (Eat("||")) {
      Value right = ParseAnd(); Node();
      left = [left,right](const evtdata &e){return double(left(e)!=0.0 || right(e)!=0.0);};
    }
    return left;
  }
  Value ParseAnd() {
    Value left = ParseCompare();
    while (Eat("&&")) {
      Value right = ParseCompare(); Node();
      left = [left,right](const evtdata &e){return double(left(e)!=0.0 && right(e)!=0.0);};
    }
    return left;
  }
  Value ParseCompare() {
    Value left = ParseSum();
    const char *ops[] = {"==","!=",">=","<=",">","<"};
    for (int op=0;op<6;++op) {
      if (!Eat(ops[op])) continue;
      Value right = ParseSum(); Node();
      return [left,right,op](const evtdata &e)->double {
        double a=left(e), b=right(e);
        switch(op) {
          case 0: return double(a==b); case 1: return double(a!=b);
          case 2: return double(a>=b); case 3: return double(a<=b);
          case 4: return double(a>b); default: return double(a<b);
        }
      };
    }
    return left;
  }
  Value ParseSum() {
    Value left = ParseProduct();
    while (true) {
      if (Eat("+")) {
        Value right=ParseProduct(); Node();
        left=[left,right](const evtdata &e){return left(e)+right(e);};
      } else if (Eat("-")) {
        Value right=ParseProduct(); Node();
        left=[left,right](const evtdata &e){return left(e)-right(e);};
      } else break;
    }
    return left;
  }
  Value ParseProduct() {
    Value left = ParseUnary();
    while (true) {
      if (Eat("*")) {
        Value right=ParseUnary(); Node();
        left=[left,right](const evtdata &e){return left(e)*right(e);};
      } else if (Eat("/")) {
        Value right=ParseUnary(); Node();
        left=[left,right](const evtdata &e){return left(e)/right(e);};
      } else break;
    }
    return left;
  }
  Value ParseUnary() {
    if (Eat("+")) return ParseUnary();
    if (Eat("-")) { Value v=ParseUnary(); Node();return [v](const evtdata &e){return -v(e);}; }
    if (Eat("!")) { Value v=ParseUnary(); Node();return [v](const evtdata &e){return double(v(e)==0.0);}; }
    return ParsePrimary();
  }
  Value ParsePrimary() {
    Space();
    if (Eat("(")) {
      Value nested=ParseOr();
      if (!Eat(")")) throw std::invalid_argument("Missing ')' in event expression");
      return nested;
    }
    if (pos_ >= source_.size()) throw std::invalid_argument("Unexpected end of event expression");
    const unsigned char first = static_cast<unsigned char>(source_[pos_]);
    if (std::isdigit(first) || first == '.') {
      const char *firstChar=source_.c_str()+pos_;
      char *end=nullptr;
      double constant=std::strtod(firstChar,&end);
      if (end == firstChar || !std::isfinite(constant))
        throw std::invalid_argument("Invalid number in event expression");
      pos_ += static_cast<std::size_t>(end-firstChar);
      Node();return [constant](const evtdata&){return constant;};
    }
    if (!std::isalpha(first) && first != '_')
      throw std::invalid_argument("Unexpected character in event expression: " + source_.substr(pos_,1));
    std::size_t start=pos_++;
    while (pos_ < source_.size()) {
      unsigned char c=static_cast<unsigned char>(source_[pos_]);
      if (std::isalnum(c) || c=='_' || c=='.' || c=='[' || c==']') ++pos_;
      else break;
    }
    Node();return ResolveVariable(source_.substr(start,pos_-start));
  }
};

inline Value CompileValue(const std::string &expression) {
  return Parser(expression).Compile();
}
inline Valid CompileCondition(const std::string &expression) {
  if (expression.empty() || expression == "always") return Valid();
  Value fn = CompileValue(expression);
  return [fn](const evtdata &e) { const double v=fn(e); return std::isfinite(v) && v != 0.0; };
}
} // namespace enana_expression
#endif
