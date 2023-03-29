// Copyright 2023, Tomasz Bold
// https://github.com/tboldagh/FunRootAna
// Distributed under the MIT License
// (See accompanying file LICENSE file)

#include "Testing.h"
#include <stdlib.h>
#include "Conf.h"
int main() {
    setenv("ENV1", "17", 0);
    Conf c("");
    VALUE( c.get<std::string>("ENV1", "missing")) EXPECTED ( "17" );
    VALUE( c.get<int>("ENV2", 0)) EXPECTED ( 0 );
    VALUE( c.get<int>("ENV1", 0)) EXPECTED ( 17 );

    VALUE( c.get<std::string>("ENV2", "missing")) EXPECTED ( "missing" );

}