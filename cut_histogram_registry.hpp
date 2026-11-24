#ifndef ENANA_CUT_HISTOGRAM_REGISTRY_HPP
#define ENANA_CUT_HISTOGRAM_REGISTRY_HPP

#include "evtdata.hpp"
#include <functional>
#include <string>
#include <vector>

class TH1;
class TH2;

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
  };

  void Register1D(TH1 *hist, const std::string &httpGroup,
                  const std::string &xExpression, Value xValue,
                  Valid valid = Valid(), const std::string &condition = "always");

  void Register2D(TH2 *hist, const std::string &httpGroup,
                  const std::string &xExpression, Value xValue,
                  const std::string &yExpression, Value yValue,
                  Valid valid = Valid(), const std::string &condition = "always");

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
