#pragma once
#include <fstream>
#include <functional>
#include <limits>
#include <optional>
#include <sstream>
#include <tuple>

#include "Conf.h"
namespace CSV {
/** @brief describes one data element in each line of CSV file
 The names are needed if "by name access" is desired, otherwise can be skipped
*/
class Item {
public:
  Item(std::string_view name, char delim = ',')
      : m_name(name), m_delim(delim) {}
  void setDelim(char d) { m_delim = d; }
  inline const std::string &name() const { return m_name; }
  template <typename T> inline std::optional<T> get() const {
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
  Record(char delim, I... items) : m_items(items...) { setDelim(delim); }

  // when each item defines delimeter
  Record(I... items) : m_items(items...) {}

  size_t size() const { return sizeof...(I); }

  template <size_t el = 0> std::istream &load(std::istream &istr) {
    if constexpr (el == sizeof...(I)) {
    } else {
      std::get<el>(m_items).load(istr);
      load<el + 1>(istr);
    }
    return istr;
  }

  template <size_t el = 0> const Item &get(std::string_view name) const {
    if constexpr (el == sizeof...(I)) {
    } else {
      const auto &item = std::get<el>(m_items);
      if (item.name() == name) {
        return item;
      }
      return get<el + 1>(name);
    }
    const static Item unknown("UNKNOWN");
    return unknown;
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

private:
  std::tuple<I...> m_items;
  const size_t m_size = sizeof...(I);

  template <size_t el = 0> void setDelim(char delim) {
    if constexpr (el == sizeof...(I)) {
    } else {
      std::get<el>(m_items).setDelim(delim);
      setDelim<el + 1>(delim);
    }
  }
};
} // namespace CSV

// the record that can be red from the file header
// struct DynamicRecord {
// };

// // read the record
template <typename... I>
std::istream &operator>>(std::istream &istr, CSV::Record<I...> &r) {
  return r.load(istr);
}

template <typename R> class CSVAccess {
public:
  CSVAccess(R record) : m_record(std::move(record)) {}
  CSVAccess(R record, std::istream &istr)
      : m_record(std::move(record)), m_istr(istr) {
    loadRecord();
  }

  void pointto(std::istream &istr) {
    m_istr = istr;
    loadRecord();
  }

  template <typename T> std::optional<T> get(std::string_view name) const {
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
