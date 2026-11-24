#ifndef ENANA_CUT_HISTOGRAM_REGISTRY_HPP
#define ENANA_CUT_HISTOGRAM_REGISTRY_HPP

#include "evtdata.hpp"
#include <functional>
#include <string>
#include <vector>

class TH1;
class TH2;
class TH1F;
class TH2F;

// A registered histogram has one well-defined (x[,y]) point per valid event.
// Label strings are metadata, while the C++ callbacks extract the real values.
class CutHistogramRegistry {
public:
  using Value = std::function<double(const evtdata&)>;
  using Valid = std::function<bool(const evtdata&)>;

  struct Entry {
    std::string name, title, path, xExpression, yExpression, condition;
    int dimension;
    TH1 *hist1d = nullptr;
    TH2 *hist2d = nullptr;
    Value xValue, yValue;
    Valid valid;
    bool manualFill = false; // special multi-point-per-event histogram; no auto Fill or cut
    bool booked = false; // true only for histograms made by Book1D/Book2D
  };

  void Register1D(TH1 *hist, const std::string &httpGroup,
                  const std::string &xExpression, Value xValue,
                  Valid valid = Valid(), const std::string &condition = "always");

  void Register2D(TH2 *hist, const std::string &httpGroup,
                  const std::string &xExpression, Value xValue,
                  const std::string &yExpression, Value yValue,
                  Valid valid = Valid(), const std::string &condition = "always");

  // Single-call API for ordinary, one-point-per-event histograms.
  // Creates the ROOT histogram, registers event-value extraction and metadata,
  // and marks it for automatic Web publishing, Reset(), and ROOT writing.
  // The returned pointer is optional: no analysis.hpp member is required.
  TH1F *Book1D(const std::string &name, const std::string &title,
               const std::string &httpGroup,
               int nBinsX, double xMin, double xMax,
               const std::string &xExpression, Value xValue,
               Valid valid = Valid(), const std::string &condition = "always");
  TH2F *Book2D(const std::string &name, const std::string &title,
               const std::string &httpGroup,
               int nBinsX, double xMin, double xMax,
               int nBinsY, double yMin, double yMax,
               const std::string &xExpression, Value xValue,
               const std::string &yExpression, Value yValue,
               Valid valid = Valid(), const std::string &condition = "always");

  // String-only booking. A restricted expression compiler resolves variable
  // names and arithmetic once at startup (never per event).
  // The legacy callback overloads above remain available for custom logic.
  TH1F *Book1D(const std::string &name, const std::string &title,
               const std::string &httpGroup,
               int nBinsX, double xMin, double xMax,
               const std::string &xExpression,
               const std::string &condition = "always");
  TH2F *Book2D(const std::string &name, const std::string &title,
               const std::string &httpGroup,
               int nBinsX, double xMin, double xMax,
               int nBinsY, double yMin, double yMax,
               const std::string &xExpression, const std::string &yExpression,
               const std::string &condition = "always");

  // Book an existing-style histogram with a dedicated multi-fill loop.
  // It is published, reset, and written automatically but cannot serve as a
  // single-point event cut. This avoids an incorrect extra auto Fill.
  TH1F *BookManual1D(const std::string &name, const std::string &title,
                     const std::string &httpGroup,
                     int nBinsX, double xMin, double xMax);
  TH2F *BookManual2D(const std::string &name, const std::string &title,
                     const std::string &httpGroup,
                     int nBinsX, double xMin, double xMax,
                     int nBinsY, double yMin, double yMax);

  // Returns false for an unregistered histogram, an event excluded by
  // its fill condition, or non-finite coordinates.
  bool GetEventPoint(const std::string &histogramName, const evtdata &event,
                     double &x, double &y) const;

  // Uses the SAME callbacks/predicates used by GetEventPoint(). Each
  // registered histogram is filled at most once per valid event.
  void FillAll(const evtdata &event) const;

  // JSON array for the editor. Values are plain metadata, never executable code.
  std::string ToJson() const;
  const std::vector<Entry>& Entries() const { return entries_; }

private:
  bool Evaluate(const Entry &entry, const evtdata &event,
                double &x, double &y) const;
  void Append(Entry entry);
  std::vector<Entry> entries_;
};

#endif
