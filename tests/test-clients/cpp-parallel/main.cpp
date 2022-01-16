/*
    SPDX-FileCopyrightText: Milian Wolff <milian.wolff@kdab.com>
    SPDX-FileCopyrightText: 2016-2022 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <cmath>
#include <complex>
#include <future>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

using namespace std;

double worker()
{
    uniform_real_distribution<double> uniform(-1E5, 1E5);
    default_random_engine engine;
    double s = 0;
    for (int i = 0; i < 10000000; ++i) {
        s += norm(complex<double>(uniform(engine), uniform(engine)));
    }
    cout << s << endl;
    return s;
}

int main(int argc, char** argv)
{
    const int numTasks = argc > 1 ? stoi(argv[1]) : std::thread::hardware_concurrency();
    vector<std::future<double>> results;
    for (int i = 0; i < numTasks; ++i) {
        results.push_back(async(launch::async, worker));
    }
    return 0;
}
