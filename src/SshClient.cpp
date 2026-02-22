#include "SshClient.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <vector>

SshClient::SshClient(const std::string &host)
    : host(host), session(ssh_new()), connected(false), failedAttempts(0) {
  if (session == nullptr) {
    std::cerr << "Failed to create SSH session for " << host << std::endl;
  }
}

SshClient::~SshClient() {
  disconnect();
  if (session) {
    ssh_free(session);
  }
}

bool SshClient::connect() {
  if (!session)
    return false;

  if (failedAttempts >= 3) {
    return false; // Skip connecting if we've already failed 3 times
  }

  // Set timeout to 10 seconds
  long timeout = 10;
  ssh_options_set(session, SSH_OPTIONS_TIMEOUT, &timeout);
  ssh_options_set(session, SSH_OPTIONS_HOST, host.c_str());

  // Connect to server
  int rc = ssh_connect(session);
  if (rc != SSH_OK) {
    failedAttempts++;
    return false;
  }

  // Authenticate with public key (assumes ~/.ssh/id_rsa or similar is set up)
  rc = ssh_userauth_publickey_auto(session, NULL, NULL);
  if (rc != SSH_AUTH_SUCCESS) {
    ssh_disconnect(session);
    failedAttempts++;
    return false;
  }

  failedAttempts = 0; // Reset on success
  connected = true;
  return true;
}

void SshClient::disconnect() {
  if (connected && session) {
    ssh_disconnect(session);
    connected = false;
  }
}

bool SshClient::isConnected() const { return connected; }

std::string SshClient::executeCommand(const std::string &command) {
  if (!connected)
    return "Error: Not connected";

  ssh_channel channel = ssh_channel_new(session);
  if (channel == nullptr)
    return "Error: Channel creation failed";

  int rc = ssh_channel_open_session(channel);
  if (rc != SSH_OK) {
    ssh_channel_free(channel);
    return "Error: Session opening failed";
  }

  rc = ssh_channel_request_exec(channel, command.c_str());
  if (rc != SSH_OK) {
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    return "Error: Command execution failed";
  }

  char buffer[256];
  int nbytes;
  std::string result = "";

  while ((nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0)) > 0) {
    result.append(buffer, nbytes);
  }

  ssh_channel_send_eof(channel);
  ssh_channel_close(channel);
  ssh_channel_free(channel);

  // Remove all newlines and carriage returns
  result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
  result.erase(std::remove(result.begin(), result.end(), '\r'), result.end());

  return result;
}

HostMetrics SshClient::fetchMetrics() {
  HostMetrics metrics;
  // Command combinations to get cleanly formatted outputs
  metrics.cpuType = executeCommand(
      "lscpu | grep 'Model name:' | awk -F ':' '{print $2}' | xargs");
  metrics.ramInfo =
      executeCommand("free -m | awk 'NR==2{printf \"%.2fGB/%.2fGB (%.2f%%)\", "
                     "$3/1024, $2/1024, $3*100/$2}'");
  metrics.diskInfo = executeCommand(
      "df -h / | awk 'NR==2{printf \"%s/%s (%s)\", $3, $2, $5}'");
  metrics.cpuUtilization =
      executeCommand("top -bn1 | grep 'Cpu(s)' | sed 's/.*, *\\([0-9.]*\\)%* "
                     "id.*/\\1/' | awk '{print 100 - $1\"%\"}'");

  return metrics;
}
