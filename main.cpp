#include <iostream>
#include <syncstream>
#include <thread>
#include <latch>
#include <chrono>
#include <vector>
#include <iomanip>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

using namespace std::chrono_literals;

void f(char x, int i) {
    std::this_thread::sleep_for(1s);
    std::osyncstream(std::cout) << "З набору " << x << " виконано дію " << i << ".\n";
}


std::latch La{ 6 }, Lb{ 5 }, Lc{ 8 }, Ld{ 5 }, Le{ 8 }, Lf{ 9 }, Lg{ 4 }, Lh{ 5 }, Li{ 6 }, Lk{ 4 };

std::latch& L(char x) {
    switch (x) {
    case 'a': return La; case 'b': return Lb; case 'c': return Lc;
    case 'd': return Ld; case 'e': return Le; case 'f': return Lf;
    case 'g': return Lg; case 'h': return Lh; case 'i': return Li;
    default:  return Lk; 
    }
}


const char* deps(char x) {
    switch (x) {
    case 'e': case 'g': return "a";
    case 'h': case 'i': return "b";
    case 'k':           return "e";
    case 'c':           return "gh";
    case 'd':           return "i";
    case 'f':           return "kcd";
    default:            return "";   
    }
}


void run(char x, int i) {
    for (const char* p = deps(x); *p; ++p) L(*p).wait();
    f(x, i);
    L(x).count_down();
}


struct Step { char set; int idx; };

const std::vector<std::vector<Step>> plans = {
    // Потік 1
    {{'a',1},{'a',6},{'b',5},{'e',5},{'k',1},{'g',3},{'h',4},{'i',4},{'c',3},{'c',6},{'d',5},{'f',1},{'f',6}},
    // Потік 2
    {{'a',2},{'b',1},{'e',1},{'e',6},{'k',2},{'g',4},{'h',5},{'i',5},{'c',4},{'c',7},{'f',2},{'f',7}},
    // Потік 3
    {{'a',3},{'b',2},{'e',2},{'e',7},{'k',3},{'h',1},{'i',1},{'i',6},{'c',5},{'c',8},{'f',3},{'f',8}},
    // Потік 4
    {{'a',4},{'b',3},{'e',3},{'e',8},{'k',4},{'h',2},{'i',2},{'c',1},{'d',1},{'d',3},{'f',4},{'f',9}},
    // Потік 5
    {{'a',5},{'b',4},{'e',4},{'g',1},{'g',2},{'h',3},{'i',3},{'c',2},{'d',2},{'d',4},{'f',5}}
};

void worker(const std::vector<Step>& plan) {
    for (const auto& s : plan) run(s.set, s.idx);
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    constexpr int nt = 5;
    static_assert(nt == 5, "nt for variant 1");

    std::osyncstream(std::cout) << "Обчислення розпочато.\n";
    auto start = std::chrono::steady_clock::now();
    {
        std::vector<std::jthread> threads;
        threads.reserve(nt);
        for (int t = 0; t < nt; ++t)
            threads.emplace_back(worker, std::cref(plans[t]));
    } 

    double sec = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    std::osyncstream(std::cout) << "Обчислення завершено. Тривалість обчислення "
        << std::fixed << std::setprecision(2) << sec << " секунд.\n";
}