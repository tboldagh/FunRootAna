#pragma once
#include <fstream>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <sstream>
#include <tuple>
#include <vector>

#include "Conf.h"
namespace CSV {
/** @brief describes one data element in each line of CSV file
 The names are needed if "by name access" is desired, otherwise can be skipped
*/
class Item {
public:
  Item(const std::string& name, char delim = ',')
      : m_name(name), m_delim(delim) {}
  void setDelim(char d) { m_delim = d; }
  inline const std::string &name() const { return m_name; }
  template <typename T> inline std::optional<T> get() const {
    if ("UNKNOWN" == m_name ) return {};
    try {
      return std::optional(strdataconv::convertTo<T>::from(m_str));
    } catch (const std::invalid_argument &) {
      return {};
    }
  }

  inline std::istream &load(std::istream &istr) {
    std::getline(istr, m_str, m_delim);
    return istr;
  }

private:
  std::string m_name;
  std::string m_str;
  char m_delim;
};
/**
 * @brief a special item that facilitate skipping items in the record
 *
 */
class Skip : public Item {
public:
  /**
   * @brief Construct a new Skip object
   *
   * @param nfields - how many fields is skipped
   * @param delim - delimeter
   */
  Skip(size_t nfields = 1, char delim = ',')
      : Item("", delim), m_fields(nfields) {}

  inline std::istream &load(std::istream &istr) {
    for (size_t i = 0; i < m_fields; ++i) {
      istr.ignore(std::numeric_limits<std::streamsize>::max(), m_delim);
    }
    return istr;
  }

private:
  size_t m_fields;
  char m_delim = ',';
};

/**
 * @brief compile time record (one CSV file line) definition
 *
 */
template <typename... I> class Record {
public:
  // record with predefined common delimeter (different from ,)
  Record(char delim, I... items) : m_items(items...) {
    setDelim(delim);
    fillNamesMap();
  }

  // when each item defines delimeter
  Record(I... items) : m_items(items...) { fillNamesMap(); }

  size_t size() const { return sizeof...(I); }

  template <size_t el = 0> std::istream &load(std::istream &istr) {
    if constexpr (el == sizeof...(I)) {
    } else {
      std::get<el>(m_items).load(istr);
      load<el + 1>(istr);
    }
    return istr;
  }

  std::istream &loadHeader(std::istream &istr) { return istr; }

  template <size_t el = 0> const Item &get(const std::string& name) const {
    auto iterator = m_nameToIndexMap.find(name);
    if (iterator == m_nameToIndexMap.end()) {
      const static Item unknown("UNKNOWN");
      return unknown;
    }
    return get(iterator->second);
  }

  template <size_t el = 0> const Item &get(size_t index) const {
    if constexpr (el == sizeof...(I)) {
    } else {
      if (index == el) {
        return std::get<el>(m_items);
      }
      return get<el + 1>(index);
    }
    const static Item unknown("UNKNOWN");
    return unknown;
  }

  template <size_t index, size_t el = 0> const Item &get() const {
    if constexpr (el == sizeof...(I)) {
    } else {
      if (index == el) {
        return std::get<el>(m_items);
      }
      return get<el + 1>(index);
    }
    const static Item unknown("UNKNOWN");
    return unknown;
  }

  bool ready() const { return true; }

private:
  std::tuple<I...> m_items;
  const size_t m_size = sizeof...(I);
  std::map<std::string, size_t> m_nameToIndexMap;

  template <size_t el = 0> void fillNamesMap() {
    if constexpr (el == sizeof...(I)) {
    } else {
      m_nameToIndexMap[std::get<el>(m_items).name()] = el;
      fillNamesMap<el + 1>();
    }
  }

  template <size_t el = 0> void setDelim(char delim) {
    if constexpr (el == sizeof...(I)) {
    } else {
      std::get<el>(m_items).setDelim(delim);
      setDelim<el + 1>(delim);
    }
  }
};
} // namespace CSV

// read the record
template <typename... I>
std::istream &operator>>(std::istream &istr, CSV::Record<I...> &r) {
  return r.load(istr);
}

namespace CSV {
// the record that can be red from the file header
class DynamicRecord {
public:
  DynamicRecord(char delim) : m_delim(delim) {}

  std::istream &load(std::istream &istr) {
    for (auto &i : m_items) {
      i.load(istr);
    }
    return istr;
  }

  std::istream &loadHeader(std::istream& istr) {
    std::string headerline;
    std::getline(istr, headerline);
    std::istringstream histr(headerline);
    while (not histr.eof()) {
      std::string name;
      std::getline(histr, name, m_delim);
      m_items.push_back(Item(name, m_delim));
      m_nameToIndexMap.insert({name,m_items.size()-1});
    }
    return istr;
  }

  bool ready() const { return not m_items.empty(); }

  const Item &get(size_t index) const {
    if ( index < m_items.size() )
      return m_items[index];
    const static Item unknown("UNKNOWN");
    return unknown;
  }

  const Item &get(const std::string& name) const {
    auto iterator = m_nameToIndexMap.find(name);
    if (iterator == m_nameToIndexMap.end()) {
      const static Item unknown("UNKNOWN");
      return unknown;
    }
    return get(iterator->second);
  }

private:
  char m_delim;
  std::vector<Item> m_items;
  std::map<std::string, size_t> m_nameToIndexMap;
};

} // EOF namespace CSV

// read the record
std::istream &operator>>(std::istream &istr, CSV::DynamicRecord &r) {
  return r.load(istr);
}


template <typename R> class CSVAccess {
public:
  /**
   * @brief Construct a new CSVAccess object
   *
   * @param record - definition of the CSV columns structure
   */
  CSVAccess(R record) : m_record(std::move(record)) {}
  /**
   * @brief Construct a new CSVAccess object
   *
   * @param record - definition of the CSV structure
   * @param istr - open input stream
   */
  CSVAccess(R record, std::istream &istr)
      : m_record(std::move(record)), m_istr(istr) {
    m_record.loadHeader(istr);
    loadRecord();
  }

  /**
   * @brief point the reader to the stream
   *
   * @param istr open input stream
   */
  void pointto(std::istream &istr) {
    m_istr = istr;
    loadRecord();
  }

  template <typename T> std::optional<T> get(const std::string& name) const {
    return m_record.get(name).template get<T>();
  }

  template <typename T> std::optional<T> get(size_t index) const {
    return m_record.get(index).template get<T>();
  }

  template <size_t index, typename T> std::optional<T> get() const {
    return m_record.template get<index>().template get<T>();
  }

  // looping helpers
  // they allow to process the tree in a loop like this:
  // MyCSVAccess csv_access(Record(...));
  // for ( csv_access.pointto(std::ifstream("file.csv")); csv_access;
  // ++csv_access)
  // or
  // for (MyCSVAccess csv_access(recordef, std::ifstream(..); csv_access;
  // ++csv_access);
  operator bool() const {
    return not(m_istr.get().eof() or m_istr.get().bad());
  }
  void operator++() { loadRecord(); }

private:
  R m_record;
  std::reference_wrapper<std::istream> m_istr;
  std::string m_line;
  std::istringstream m_linestr;
  void loadRecord() {
    std::getline(m_istr.get(), m_line);
    m_linestr.clear();
    m_linestr.str(m_line);
    m_linestr >> m_record;
  }
};
