// Copyright 2024, Tomasz Bold
// https://github.com/tboldagh/FunRootAna
// Distributed under the MIT License
// (See accompanying file LICENSE file)
#ifndef SELECTOR_H
#define SELECTOR_H
// helper to streamline selection from set of values according to a boolean condition
// e.g const double val   = option(condA, 0.2)
//                         .option(condB, 0.3)
//                         .option(condC, 1.1)
//                         .option(0.1) // if none of the above applies
//                         .select();
// raises, if no condition is satisfied
template<typename T = double>
struct Selector {
    Selector<T> option( bool cond, T value) {
        if (not _selected and cond ) {
            _value = value;
            _selected = true;
        }
        return *this;
    }
    Selector<T> option( T value ) {
        if ( not _selected ) {
            _value = value;
            _selected = true;
        }
        return *this;
    }

    T select(){ if ( not _selected ) throw std::runtime_error("No option is selected"); return _value; }
    T _value = {};
    bool _selected = {false};
};

template<typename T = double>
Selector<T> option(bool cond, T value) {
    Selector<T> s;
    s.option(cond, value);
    return s;
}



#endif