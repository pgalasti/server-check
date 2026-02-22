#include "ConfigManager.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

ConfigManager::ConfigManager() {
  struct passwd *pw = getpwuid(getuid());
  std::string homeDir = pw->pw_dir;

  std::string configDir = homeDir + "/.config/server-check";
  configFilePath = configDir + "/hosts.txt";

  ensureConfigDirExists();
  loadHosts();
}

void ConfigManager::ensureConfigDirExists() {
  struct passwd *pw = getpwuid(getuid());
  std::string configDir = std::string(pw->pw_dir) + "/.config/server-check";

  struct stat st = {0};
  if (stat(configDir.c_str(), &st) == -1) {
    mkdir(configDir.c_str(), 0700);
  }
}

void ConfigManager::loadHosts() {
  std::ifstream file(configFilePath);
  if (!file.is_open())
    return;

  std::string line;
  while (std::getline(file, line)) {
    if (!line.empty()) {
      hosts.push_back(line);
    }
  }
}

void ConfigManager::saveHosts() {
  std::ofstream file(configFilePath);
  if (!file.is_open()) {
    std::cerr << "Failed to open config file for writing.\n";
    return;
  }

  for (const auto &host : hosts) {
    file << host << "\n";
  }
}

bool ConfigManager::addHost(const std::string &host) {
  if (std::find(hosts.begin(), hosts.end(), host) != hosts.end()) {
    std::cerr << "Host " << host << " already exists.\n";
    return false;
  }

  hosts.push_back(host);
  saveHosts();
  std::cout << "Successfully added host: " << host << "\n";
  return true;
}

bool ConfigManager::removeHost(const std::string &host) {
  auto it = std::find(hosts.begin(), hosts.end(), host);
  if (it != hosts.end()) {
    hosts.erase(it);
    saveHosts();
    std::cout << "Successfully removed host: " << host << "\n";
    return true;
  }

  std::cerr << "Host " << host << " not found.\n";
  return false;
}

std::vector<std::string> ConfigManager::getHosts() const { return hosts; }
