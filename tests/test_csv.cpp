// Copyright 2023, Tomasz Bold
// https://github.com/tboldagh/FunRootAna
// Distributed under the MIT License
// (See accompanying file LICENSE file)

#include "Testing.h"
#include "CSVAccess.h"
#include <sstream>
#include <vector>
#include "LazyFunctionalVector.h"

using namespace CSV;

void test_items() {
    std::string s("9");
    std::istringstream i(s);
    Item x("x");
    x.load(i);
    VALUE(x.get<int>().has_value()) EXPECTED(true);
    VALUE(x.get<int>().value()) EXPECTED(9);
    Item y("y");
    y.load(i);
    VALUE(y.get<float>().has_value()) EXPECTED(false);
}
void test_record_access_by_name() {
    std::string s("1,3.34,hello");
    std::istringstream i(s);
    Record r(',', Item("x"), Item("d"), Item("label"));
    i >> r;
    VALUE(r.size()) EXPECTED(3);
    VALUE(r.get("sth").name()) EXPECTED("UNKNOWN");
    VALUE(r.get("label").name()) EXPECTED("label");
    VALUE(r.get("label").get<std::string>().value()) EXPECTED("hello");

    VALUE(r.get("d").name()) EXPECTED("d");
    VALUE(r.get("d").get<float>().value()) EXPECTED(3.34);

    VALUE(r.get("x").name()) EXPECTED("x");
    VALUE(r.get("x").get<int>().value()) EXPECTED(1);
}

void test_record_access_by_index() {
    std::string s("1,3.34,hello");
    std::istringstream i(s);
    Record r(',', Item("x"), Item("d"), Item("label"));
    i >> r;
    VALUE(r.size()) EXPECTED(3);

    // index known at runtime
    VALUE(r.get(7).name()) EXPECTED("UNKNOWN");
    VALUE(r.get(2).name()) EXPECTED("label");
    VALUE(r.get(2).get<std::string>().value()) EXPECTED("hello");

    // compile time known index
    VALUE(r.get<7>().name()) EXPECTED("UNKNOWN");
    VALUE(r.get<2>().name()) EXPECTED("label");
    VALUE(r.get<2>().get<std::string>().value()) EXPECTED("hello");
}


void test_skip() {
    std::string s("1,3.34,me,you,4,23.09,hello");
    std::istringstream i(s);
    Record r(',', Item("x"), Skip(2), Item("who"), Skip(), Item("grad"), Item("label"));
    i >> r;
    VALUE(r.get("sth").name()) EXPECTED("UNKNOWN");
    VALUE(r.get("label").name()) EXPECTED("label");
    VALUE(r.get("label").get<std::string>().value()) EXPECTED("hello");

    VALUE(r.get("x").get<float>().value()) EXPECTED(1);
    VALUE(r.get("who").get<std::string>().value()) EXPECTED("you");
    VALUE(r.get("grad").get<double>().value()) EXPECTED(23.09);
    VALUE(r.get("label").get<std::string>().value()) EXPECTED("hello");
}

void test_access() {
    std::string s("1,3.34,hello\n2,0.34,people\n3,1.34,there\n");
    const auto xdef = std::vector<int>({1, 2, 3});
    const auto ddef = std::vector<float>({3.34, 0.34, 1.34});
    const auto sdef = std::vector<std::string>({"hello", "people", "there"});

    std::istringstream i(s);
    Record r(Item("x"), Item("d"), Item("label"));
    CSVAccess acc(r, i);
    VALUE(static_cast<bool>(acc)) EXPECTED(true);
    size_t count = 0;
    for ( ; acc; ++acc, ++count) {
        VALUE(acc.get<int>("x").has_value()) EXPECTED(true);
        VALUE(acc.get<int>("x").value()) EXPECTED(xdef[count]);
        VALUE(acc.get<float>("d").has_value()) EXPECTED(true);
        VALUE(acc.get<float>("d").value()) EXPECTED(ddef[count]);
        VALUE(acc.get<std::string>("label").has_value()) EXPECTED(true);
        VALUE(acc.get<std::string>("label").value()) EXPECTED(sdef[count]);
    }
    VALUE(count) EXPECTED(3);
}

void test_view() {
    std::string s("1,3.34,hello\n2,0.34,people\n3,1.34,there\n");
    std::istringstream i(s);
    Record r(Item("x"), Item("d"), Item("label"));
    CSVAccess acc(r, i);
    AccessView view(acc);
    int sum=0;
    std::string concat;
    view.foreach( [&sum,&concat](auto& r) {
        sum += r.template get<int>("x").value();
        concat += r.template get<std::string>("label").value();

    });
    VALUE(sum) EXPECTED(6);
    VALUE(concat) EXPECTED("hellopeoplethere");
}

void test_dyamic() {
    std::string s("x,d,label\n1,3.34,hello\n2,0.34,people\n3,1.34,there\n");
    const auto xdef = std::vector<int>({1, 2, 3});
    const auto ddef = std::vector<float>({3.34, 0.34, 1.34});
    const auto sdef = std::vector<std::string>({"hello", "people", "there"});
    std::istringstream i(s);
    CSVAccess acc(DynamicRecord(','), i);

    size_t count = 0;
    for ( ; acc; ++acc, ++count) {
        VALUE(acc.get<int>("x").has_value()) EXPECTED(true);
        VALUE(acc.get<int>("x").value()) EXPECTED(xdef[count]);
        VALUE(acc.get<float>("d").has_value()) EXPECTED(true);
        VALUE(acc.get<float>("d").value()) EXPECTED(ddef[count]);
        VALUE(acc.get<std::string>("label").has_value()) EXPECTED(true);
        VALUE(acc.get<std::string>("label").value()) EXPECTED(sdef[count]);
        VALUE(acc.get<std::string>("info").has_value()) EXPECTED(false);

    }
    VALUE(count) EXPECTED(3);
}

int main() {
        const int failed =
            SUITE(test_items) +
            SUITE(test_record_access_by_name) +
            SUITE(test_record_access_by_index) +
            SUITE(test_skip) +
            SUITE(test_access) +
            SUITE(test_view)+
            SUITE(test_dyamic);
    std::cout << (failed == 0 ? "ALL OK" : "FAILURE") << std::endl;
    return failed;

    // CSVAccess access(i, Record(',', ));
}
