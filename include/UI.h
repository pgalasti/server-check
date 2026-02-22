#pragma once

#include "ConfigManager.h"
#include "SshClient.h"
#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

class UI {
public:
  UI(ConfigManager &configManager);
  ~UI();

  void run();

private:
  ConfigManager &config;
  std::vector<std::unique_ptr<SshClient>> clients;
  std::map<std::string, HostMetrics> latestMetrics;

  std::atomic<bool> running;
  std::mutex metricsMutex;
  std::thread updateThread;

  void drawScreen();
  void updateLoop();
  void setupNcurses();
  void cleanupNcurses();
};
