// Copyright 2022, Tomasz Bold
// https://github.com/tboldagh/FunRootAna
// Distributed under the MIT License
// (See accompanying file LICENSE file)
#include <iostream>
#include "Testing.h"
#include "Weights.h"

void test() {
    VALUE(WEIGHT) EXPECTED(1.0);
    {
        UPDATE_ABS_WEIGHT(0.3);
        {
            VALUE(WEIGHT) EXPECTED(0.3);
        }
    }
    VALUE(WEIGHT) EXPECTED(1.0);

    {
        UPDATE_ABS_WEIGHT(2.0);
        UPDATE_MULT_WEIGHT(1.5)
        VALUE(WEIGHT) EXPECTED(3.0);
    }
    VALUE(WEIGHT) EXPECTED(1.0);
    {
        UPDATE_ABS_WEIGHT(2.0);
        {
            UPDATE_MULT_WEIGHT(1.5);
            VALUE(WEIGHT) EXPECTED(3.0);
            {
                UPDATE_MULT_WEIGHT(3.0);
                {
                    VALUE(WEIGHT) EXPECTED(9.0);
                    UPDATE_ABS_WEIGHT(0.6);
                    VALUE(WEIGHT) EXPECTED(0.6);

                }
                VALUE(WEIGHT) EXPECTED(9.0);
            }
            VALUE(WEIGHT) EXPECTED(3.0);
        }
        VALUE(WEIGHT) EXPECTED(2.0);
    }
}

int main() {
    return SUITE(test);
}