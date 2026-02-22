#include <iostream>
#include <string>

#include "ConfigManager.h"
#include "UI.h"

void printUsage() {
  std::cout << "Usage:\n"
            << "  server-check               : Run the monitoring UI\n"
            << "  server-check --add <host>  : Add a host to monitor\n"
            << "  server-check --remove <host>: Remove a host\n";
}

int main(int argc, char *argv[]) {
  ConfigManager config;

  if (argc > 1) {
    std::string arg = argv[1];
    if (arg == "--add" && argc == 3) {
      std::string host = argv[2];
      config.addHost(host);
      return 0;
    } else if (arg == "--remove" && argc == 3) {
      std::string host = argv[2];
      config.removeHost(host);
      return 0;
    } else {
      printUsage();
      return 1;
    }
  }

  if (config.getHosts().empty()) {
    std::cout << "No hosts configured. Use 'server-check --add <host>' to add "
                 "some.\n";
    return 0;
  }

  UI ui(config);
  ui.run();
  return 0;
}
