// Copyright 2022, Tomasz Bold
// https://github.com/tboldagh/FunRootAna
// Distributed under the MIT License
// (See accompanying file LICENSE file)
#include <fstream>
#include "assure.h"
#include "Conf.h"

Conf::Conf(const std::string& fname) {
    if ( fname.empty() ) {
        m_useEnv = true;
        return;
    }
    
    std::ifstream f(fname);
    while(f) {
        std::string line;
        std::getline(f, line);
        if (line[0] == '#' or line.empty()) {
        } else {
            const size_t eq = line.find('=');
            assure( eq != std::string::npos, "Missing = in config line: "+line);
            const std::string key = line.substr(0, eq);
            assure( m_kvMap.find(key) == m_kvMap.end(), "Key "+ key+" present twice in config file");
            m_kvMap[key] = line.substr(eq+1);      
        }
        f.peek();
    }
    f.close();

}
