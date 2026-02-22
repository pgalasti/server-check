#include "UI.h"
#include <chrono>
#include <g-lib/util/StringParser.h>
#include <ncurses.h>
#include <unistd.h>

namespace UIConfig {
constexpr int COLOR_GREEN_ID = 1;
constexpr int COLOR_YELLOW_ID = 2;
constexpr int COLOR_RED_ID = 3;

constexpr float THRESHOLD_RED = 80.0f;
constexpr float THRESHOLD_YELLOW = 50.0f;

constexpr int ROW_INITIAL_OFFSET = 4;

constexpr int COL_HOST = 0;
constexpr int COL_CPU_TYPE = 21;
constexpr int COL_CPU_UTIL = 47;
constexpr int COL_RAM_USAGE = 63;
constexpr int COL_DISK_USAGE = 94;
} // namespace UIConfig
UI::UI(ConfigManager &configManager) : config(configManager), running(false) {
  for (const auto &host : config.getHosts()) {
    clients.push_back(std::make_unique<SshClient>(host));
  }
}

UI::~UI() {
  running = false;
  if (updateThread.joinable()) {
    updateThread.join();
  }
  cleanupNcurses();
}

void UI::setupNcurses() {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  nodelay(stdscr, TRUE); // non-blocking input

  if (has_colors()) {
    start_color();
    use_default_colors();
    init_pair(UIConfig::COLOR_GREEN_ID, COLOR_GREEN, -1);
    init_pair(UIConfig::COLOR_YELLOW_ID, COLOR_YELLOW, -1);
    init_pair(UIConfig::COLOR_RED_ID, COLOR_RED, -1);
  }

  curs_set(0); // hide cursor
}

void UI::cleanupNcurses() { endwin(); }

void UI::updateLoop() {
  while (running) {
    for (auto &client : clients) {
      if (!client->isConnected()) {
        client->connect();
      }

      if (client->isConnected()) {
        HostMetrics metrics = client->fetchMetrics();
        std::lock_guard<std::mutex> lock(metricsMutex);
        latestMetrics[client->getHost()] = metrics;
      }
    }
    std::this_thread::sleep_for(std::chrono::seconds(5));
  }
}

void UI::drawScreen() {
  clear();
  mvprintw(0, 0, "Server Check - Press 'Q' or 'q' to quit.");
  mvprintw(2, 0, "%-20s %-25s %-15s %-30s %-20s", "Host", "CPU Type",
           "CPU Util", "RAM Usage (%%)", "Disk Usage");
  mvprintw(3, 0,
           "-------------------------------------------------------------------"
           "---------------------------------------------");

  int row = UIConfig::ROW_INITIAL_OFFSET;
  std::lock_guard<std::mutex> lock(metricsMutex);

  // Iterate over configured hosts directly to keep order and show disconnected
  // ones
  for (size_t i = 0; i < config.getHosts().size(); ++i) {
    std::string host = config.getHosts()[i];
    auto *clientPtr = clients[i].get();

    if (clientPtr && clientPtr->getFailedAttempts() >= 3) {
      if (has_colors())
        attron(COLOR_PAIR(UIConfig::COLOR_RED_ID)); // Red
      mvprintw(row, UIConfig::COL_HOST, "%-20s", host.c_str());
      mvprintw(row, UIConfig::COL_CPU_TYPE, "%-25s", "Connection Failed");
      mvprintw(row, UIConfig::COL_CPU_UTIL, "%-15s", "-");
      mvprintw(row, UIConfig::COL_RAM_USAGE, "%-30s", "-");
      mvprintw(row, UIConfig::COL_DISK_USAGE, "%-20s", "-");
      if (has_colors())
        attroff(COLOR_PAIR(UIConfig::COLOR_RED_ID));
      row++;
      continue;
    }

    if (has_colors())
      attron(COLOR_PAIR(UIConfig::COLOR_GREEN_ID)); // Green for connected host
    mvprintw(row, UIConfig::COL_HOST, "%-20s", host.c_str());
    if (has_colors())
      attroff(COLOR_PAIR(UIConfig::COLOR_GREEN_ID));

    auto it = latestMetrics.find(host);
    if (it != latestMetrics.end()) {
      const HostMetrics &m = it->second;
      mvprintw(row, UIConfig::COL_CPU_TYPE, "%-25.25s", m.cpuType.c_str());

      auto getColor = [](const std::string &metric) {
        try {
          GLib::Util::StringParser parser(metric, "(");
          if (parser.getSize() > 1) {
            parser.getLast();
            float val = std::stof(parser.getToken());
            if (val > UIConfig::THRESHOLD_RED)
              return UIConfig::COLOR_RED_ID;
            if (val > UIConfig::THRESHOLD_YELLOW)
              return UIConfig::COLOR_YELLOW_ID;
          } else {
            float val = std::stof(metric);
            if (val > UIConfig::THRESHOLD_RED)
              return UIConfig::COLOR_RED_ID;
            if (val > UIConfig::THRESHOLD_YELLOW)
              return UIConfig::COLOR_YELLOW_ID;
          }
        } catch (...) {
        }
        return UIConfig::COLOR_GREEN_ID;
      };

      int colorCPU = getColor(m.cpuUtilization);
      int colorRAM = getColor(m.ramInfo);
      int colorDisk = getColor(m.diskInfo);

      if (has_colors())
        attron(COLOR_PAIR(colorCPU));
      mvprintw(row, UIConfig::COL_CPU_UTIL, "%-15.15s",
               m.cpuUtilization.c_str());
      if (has_colors())
        attroff(COLOR_PAIR(colorCPU));

      if (has_colors())
        attron(COLOR_PAIR(colorRAM));
      mvprintw(row, UIConfig::COL_RAM_USAGE, "%-30.30s", m.ramInfo.c_str());
      if (has_colors())
        attroff(COLOR_PAIR(colorRAM));

      if (has_colors())
        attron(COLOR_PAIR(colorDisk));
      mvprintw(row, UIConfig::COL_DISK_USAGE, "%-20.20s", m.diskInfo.c_str());
      if (has_colors())
        attroff(COLOR_PAIR(colorDisk));

    } else {
      mvprintw(row, UIConfig::COL_CPU_TYPE, "%-25s", "Connecting...");
      mvprintw(row, UIConfig::COL_CPU_UTIL, "%-15s", "-");
      mvprintw(row, UIConfig::COL_RAM_USAGE, "%-30s", "-");
      mvprintw(row, UIConfig::COL_DISK_USAGE, "%-20s", "-");
    }
    row++;
  }

  refresh();
}

void UI::run() {
  setupNcurses();

  running = true;
  updateThread = std::thread(&UI::updateLoop, this);

  int ch;
  while (running) {
    drawScreen();

    // Sleep in small increments to maintain responsiveness to keypresses
    for (int i = 0; i < 50; ++i) { // 50 * 100ms = 5s
      ch = getch();
      if (ch == 'q' || ch == 'Q') {
        cleanupNcurses();
        std::exit(0);
      }
      std::this_thread::sleep_for(
          std::chrono::milliseconds(100)); // Sleep 100ms
    }
  }
}
