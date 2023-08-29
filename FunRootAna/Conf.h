// Copyright 2022, Tomasz Bold
// https://github.com/tboldagh/FunRootAna
// Distributed under the MIT License
// (See accompanying file LICENSE file)
#pragma once
#include <cstdlib>
#include <map>
#include <string>
class TFile;
namespace strdataconv {
template <typename T> struct convertTo {
  static T from(const std::string &value) {
    static_assert(true, "Can't convert it");
    return {};
  }
};

template <> struct convertTo<std::string> {
  static std::string from(const std::string &value) { return value; }
};

template <> struct convertTo<const char *> {
  static std::string from(const std::string &value) { return value; }
};

template <> struct convertTo<float> {
  static float from(const std::string &value) { return std::stof(value); }
};
template <> struct convertTo<double> {
  static double from(const std::string &value) { return std::stod(value); }
};

template <> struct convertTo<int> {
  static int from(const std::string &value) { return std::stoi(value); }
};

template <> struct convertTo<bool> {
  static bool from(const std::string &value) {
    using namespace std::string_literals;
    return value == "1"s or value == "true"s or value == "True"s or
           value == "TRUE"s or value == "yes"s or value == "YES"s;
  }
};

} // namespace strdataconv

class Conf {
public:
  // reads config from a the file of a given name
  // if file argument is en empty string, the config is read from process
  // environment variables
  Conf(const std::string &fname="");
  
  template <typename T> T get(const std::string &key, const T &def) const {
    if (m_useEnv) {
      const char *val = std::getenv(key.c_str());
      if ( val ) {
        m_kvMap[key] = val;
        return strdataconv::convertTo<T>::from(val);
      }
      return def;
    } else {
      if (not has(key))
        return def;
      return strdataconv::convertTo<T>::from(m_kvMap.at(key));
    }
  }
  bool has(const std::string &key) const {
    if (m_useEnv) {
      return std::getenv(key.c_str()) != nullptr;
    } else {
      return m_kvMap.find(key) != m_kvMap.end();
    }
  }
  /**
   * @brief Opens file and adds TTree with config key & value pairs
   *
   * @param fileName file to add to
   * @param metaName metadata tree name
   */
  void saveAsMetadata(const std::string &fileName,
                      const std::map<std::string, std::string> &extra,
                      const std::string &metaName = "meta") const;

private:
  mutable std::map<std::string, std::string> m_kvMap;
  bool m_useEnv = false;
};