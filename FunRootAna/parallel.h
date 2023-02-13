#pragma once
#include <iostream>
#include <thread>
#include <utility>
// #include "BS_thread_pool_light.hpp"
// static BS::thread_pool_light thread_pool;
class parallel_scope {
public:
  parallel_scope() {
    s_threading_on = true;
    ;
    std::cerr << " enabled threading in this scope\n";
  }
  ~parallel_scope() {
    std::cerr << " waiting for taks\n";
    s_threading_on = false;
    for (auto &t : s_active_threads) {
      t.join();
    }
    s_active_threads.clear();
    std::cerr << " taks done ...\n";
  }
  static bool active_tp() { return s_threading_on; }

  static std::atomic<int> s_tcount;
  static std::atomic<bool> s_threading_on;
  static std::vector<std::thread> s_active_threads;

  // parallel_scope() { s_tp = &thread_pool; std::cerr << " enabled threading in
  // this scope\n";} ~parallel_scope() { std::cerr << " waiting for taks\n";
  // s_tp->wait_for_tasks(); s_tp = nullptr; std::cerr << " taks done ...\n";}
  // static BS::thread_pool_light * s_tp;
  // static bool active_tp() { return s_tp != nullptr; }
};
// BS::thread_pool_light* parallel_scope::s_tp = nullptr;
std::atomic<int> parallel_scope::s_tcount = 0;
std::atomic<bool> parallel_scope::s_threading_on = true;
std::vector<std::thread> parallel_scope::s_active_threads;

template <typename F> void as_threaded_task(F &&code_to_run) {
  if (parallel_scope::active_tp()) {
    parallel_scope::s_tcount++;
    parallel_scope::s_active_threads.emplace_back(
        [](F &&c) {
          c();
          parallel_scope::s_tcount--;
        },
        std::move(code_to_run));

    // thread_pool.push_task(code_to_run);
    // thread_pool.push_task(code_to_run, std::forward<ARGS>(args)...);

  } else {
    code_to_run();
  }
}

#define PARALLEL parallel_scope _tp_RAII##__LINE__;