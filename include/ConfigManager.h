#pragma once

#include <string>
#include <vector>

class ConfigManager {
public:
  ConfigManager();
  ~ConfigManager() = default;

  bool addHost(const std::string &host);
  bool removeHost(const std::string &host);
  std::vector<std::string> getHosts() const;

private:
  std::string configFilePath;
  std::vector<std::string> hosts;

  void ensureConfigDirExists();
  void loadHosts();
  void saveHosts();
};
