#pragma once

#include <libssh/libssh.h>
#include <memory>
#include <string>

struct HostMetrics {
  std::string cpuType;
  std::string ramInfo;
  std::string diskInfo;
  std::string cpuUtilization;
};

class SshClient {
public:
  SshClient(const std::string &host);
  ~SshClient();

  bool connect();
  void disconnect();
  bool isConnected() const;
  const std::string &getHost() const { return host; }
  int getFailedAttempts() const { return failedAttempts; }

  HostMetrics fetchMetrics();

private:
  std::string host;
  ssh_session session;
  bool connected;
  int failedAttempts;

  std::string executeCommand(const std::string &command);
};
