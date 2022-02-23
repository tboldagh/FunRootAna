// Copyright 2022, Tomasz Bold
// https://github.com/tboldagh/FunRootAna
// Distributed under the MIT License
// (See accompanying file LICENSE file)
#pragma once
#include <map>
#include <cstdlib>
namespace {
    template<typename T>
    struct convertData {
        static T from(const std::string& value) { static_assert(true, "Can't convert it"); return {}; }
    };

    template<>
    struct convertData<std::string> {
        static std::string from(const std::string& value) { return value; }
    };

    template<>
    struct convertData<const char*> {
        static std::string from(const std::string& value) { return value; }
    };

    template<>
    struct convertData<float> {
        static float from(const std::string& value) { return std::stof(value); }
    };
    template<>
    struct convertData<double> {
        static double from(const std::string& value) { return std::stod(value); }
    };

    template<>
    struct convertData<int> {
        static int from(const std::string& value) { return std::stoi(value); }
    };

    template<>
    struct convertData<bool> {
        static bool from(const std::string& value) {
            return value == "1" or value == "true" or value == "True" or value == "TRUE" or value == "yes" or value == "YES";
        }
    };



}
class Conf {
public:
    // reads config from a the file of a given name
    // if file argument is en empty string, the config is read from process environment variables
    Conf(const std::string& fname);
    template<typename T>
    T get(const std::string& key, const T& def) const {
        if (m_useEnv) {
            const char* val = std::getenv(key.c_str());
            return (val ? convertData<T>::from(val) : def);
        }
        else {
            if (not has(key)) return def;
            return convertData<T>::from(m_kvMap.at(key));
        }
    }
    bool has(const std::string& key) const {
        if (m_useEnv) {
            return std::getenv(key.c_str()) != nullptr;
        }
        else {
            return m_kvMap.find(key) != m_kvMap.end();
        }
    }
private:

    std::map<std::string, std::string> m_kvMap;
    bool m_useEnv = false;
};