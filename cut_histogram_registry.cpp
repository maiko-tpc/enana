#include "cut_histogram_registry.hpp"
#include <TH1.h>
#include <TH2.h>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <sstream>
#include <utility>
#include <memory>
#include <TDirectory.h>

namespace {
std::string JsonString(const std::string &value) {
  const char *digits = "0123456789abcdef";
  std::string result = "\"";
  for (unsigned char c : value) {
    switch (c) {
      case '"': result += "\\\""; break;
      case '\\': result += "\\\\"; break;
      case '\n': result += "\\n"; break;
      case '\r': result += "\\r"; break;
      case '\t': result += "\\t"; break;
      default:
        if (c < 0x20) {
          result += "\\u00";
          result += digits[c >> 4];
          result += digits[c & 15];
        } else {
          result += static_cast<char>(c);
        }
    }
  }
  result += '"';
  return result;
}

std::string HttpPath(const std::string &group, const char *name) {
  if (group.empty() || group[0] != '/' || !name || !*name)
    throw std::invalid_argument("Invalid registered histogram path");
  return group + (group.back() == '/' ? "" : "/") + name;
}
}

void CutHistogramRegistry::Append(Entry entry) {
  for (const auto &existing : entries_)
    if (existing.name == entry.name)
      throw std::invalid_argument("Duplicate cut histogram registration: " + entry.name);
  entries_.push_back(std::move(entry));
}

void CutHistogramRegistry::Register1D(TH1 *hist, const std::string &httpGroup,
                                      const std::string &xExpression, Value xValue,
                                      Valid valid, const std::string &condition) {
  if (!hist || hist->GetDimension() != 1 || !xValue || xExpression.empty())
    throw std::invalid_argument("Invalid 1D cut histogram registration");
  Entry e;
  e.name = hist->GetName();
  e.title = hist->GetTitle();
  e.path = HttpPath(httpGroup, hist->GetName());
  e.xExpression = xExpression;
  e.yExpression = "";
  e.condition = condition;
  e.dimension = 1;
  e.hist1d = hist;
  e.xValue = std::move(xValue);
  e.valid = std::move(valid);
  Append(std::move(e));
}

void CutHistogramRegistry::Register2D(TH2 *hist, const std::string &httpGroup,
                                      const std::string &xExpression, Value xValue,
                                      const std::string &yExpression, Value yValue,
                                      Valid valid, const std::string &condition) {
  if (!hist || hist->GetDimension() != 2 || !xValue || !yValue ||
      xExpression.empty() || yExpression.empty())
    throw std::invalid_argument("Invalid 2D cut histogram registration");
  Entry e;
  e.name = hist->GetName();
  e.title = hist->GetTitle();
  e.path = HttpPath(httpGroup, hist->GetName());
  e.xExpression = xExpression;
  e.yExpression = yExpression;
  e.condition = condition;
  e.dimension = 2;
  e.hist2d = hist;
  e.xValue = std::move(xValue);
  e.yValue = std::move(yValue);
  e.valid = std::move(valid);
  Append(std::move(e));
}

// Booked histograms are owned by the output ROOT TDirectory, just like
// manually created TH1F/TH2F objects in enana. Entries hold non-owning pointers.
TH1F *CutHistogramRegistry::Book1D(const std::string &name, const std::string &title,
                                   const std::string &httpGroup,
                                   int nBinsX, double xMin, double xMax,
                                   const std::string &xExpression, Value xValue,
                                   Valid valid, const std::string &condition) {
  if (name.empty() || name.find('/') != std::string::npos || nBinsX <= 0 ||
      !std::isfinite(xMin) || !std::isfinite(xMax) || xMin >= xMax)
    throw std::invalid_argument("Invalid Book1D histogram name or axis range");
  for (const auto &entry : entries_)
    if (entry.name == name)
      throw std::invalid_argument("Duplicate booked histogram: " + name);
  if (gDirectory && gDirectory->FindObject(name.c_str()))
    throw std::invalid_argument("Histogram already exists in ROOT directory: " + name);
  std::unique_ptr<TH1F> hist(new TH1F(name.c_str(), title.c_str(), nBinsX, xMin, xMax));
  // Explicitly associate the histogram with the current output directory.
  if (gDirectory) hist->SetDirectory(gDirectory);
  Register1D(hist.get(), httpGroup, xExpression, std::move(xValue),
             std::move(valid), condition);
  entries_.back().booked = true;
  return hist.release();
}

TH2F *CutHistogramRegistry::Book2D(const std::string &name, const std::string &title,
                                   const std::string &httpGroup,
                                   int nBinsX, double xMin, double xMax,
                                   int nBinsY, double yMin, double yMax,
                                   const std::string &xExpression, Value xValue,
                                   const std::string &yExpression, Value yValue,
                                   Valid valid, const std::string &condition) {
  if (name.empty() || name.find('/') != std::string::npos ||
      nBinsX <= 0 || nBinsY <= 0 ||
      !std::isfinite(xMin) || !std::isfinite(xMax) || xMin >= xMax ||
      !std::isfinite(yMin) || !std::isfinite(yMax) || yMin >= yMax)
    throw std::invalid_argument("Invalid Book2D histogram name or axis range");
  for (const auto &entry : entries_)
    if (entry.name == name)
      throw std::invalid_argument("Duplicate booked histogram: " + name);
  if (gDirectory && gDirectory->FindObject(name.c_str()))
    throw std::invalid_argument("Histogram already exists in ROOT directory: " + name);
  std::unique_ptr<TH2F> hist(new TH2F(name.c_str(), title.c_str(), nBinsX, xMin, xMax,
                                      nBinsY, yMin, yMax));
  if (gDirectory) hist->SetDirectory(gDirectory);
  Register2D(hist.get(), httpGroup, xExpression, std::move(xValue),
             yExpression, std::move(yValue), std::move(valid), condition);
  entries_.back().booked = true;
  return hist.release();
}

bool CutHistogramRegistry::Evaluate(const Entry &entry, const evtdata &event,
                                    double &x, double &y) const {
  if (entry.valid && !entry.valid(event)) return false;
  const double xv = entry.xValue(event);
  const double yv = entry.dimension == 2 ? entry.yValue(event) : 0.;
  if (!std::isfinite(xv) || !std::isfinite(yv)) return false;
  x = xv;
  y = yv;
  return true;
}

bool CutHistogramRegistry::GetEventPoint(const std::string &histogramName,
                                         const evtdata &event,
                                         double &x, double &y) const {
  for (const auto &entry : entries_) {
    if (entry.name == histogramName) return Evaluate(entry, event, x, y);
  }
  return false;
}

void CutHistogramRegistry::FillAll(const evtdata &event) const {
  for (const auto &entry : entries_) {
    double x = 0., y = 0.;
    if (!Evaluate(entry, event, x, y)) continue;
    if (entry.dimension == 1)
      entry.hist1d->Fill(x);
    else
      entry.hist2d->Fill(x, y);
  }
}

std::string CutHistogramRegistry::ToJson() const {
  std::ostringstream out;
  out << '[';
  for (std::size_t i = 0; i < entries_.size(); ++i) {
    const Entry &e = entries_[i];
    if (i) out << ',';
    out << "{\"name\":" << JsonString(e.name)
        << ",\"title\":" << JsonString(e.title)
        << ",\"path\":" << JsonString(e.path)
        << ",\"dimension\":" << e.dimension
        << ",\"x\":" << JsonString(e.xExpression)
        << ",\"y\":" << JsonString(e.yExpression)
        << ",\"condition\":" << JsonString(e.condition)
        << '}';
  }
  out << ']';
  return out.str();
}
