# Server Check

Like any good dork, I have a slew of Linux servers running around my house. 

`server-check` is a C++ CLI application that connects to a list of hosts via SSH to fetch and display system metrics (CPU type, RAM, Disk, CPU utilization) on a continuously updating `ncurses` terminal UI.

## Dependencies

To build and run this application, you will need the following dependencies installed on your system:
- `libssh-dev` (or `libssh2`)
- `libncurses5-dev` (or `ncurses`)
- A C++17 compatible compiler (e.g., `g++`)
- `g-lib`: A utility library. You can clone and install it from [https://github.com/pgalasti/g-lib](https://github.com/pgalasti/g-lib). Make sure to run `make install` for `g-lib` so it is installed in `/usr/local/include` and `/usr/local/lib`.

### TL;DR (Ubuntu)

Run the following commands to install all necessary prerequisites and `g-lib` automatically:

```bash
# Install core build tools and dependencies
sudo apt update
sudo apt install -y build-essential libssh-dev libncurses5-dev git

# Clone and install the g-lib utility library
git clone https://github.com/pgalasti/g-lib.git /tmp/g-lib
cd /tmp/g-lib
make
sudo make install
rm -rf /tmp/g-lib
```

## Building

To compile the application, simply run:
```bash
make
```
This will compile the source files and create the `server-check` executable in the `build/` directory.

## Installation

You can install the application system-wide by running:
```bash
sudo make install
```
This will copy the executable to `/usr/local/bin`.

## Usage

Before running the UI, add some hosts. The application assumes you have passwordless SSH (public key authentication) set up for these hosts.

**Adding a host:**
```bash
server-check --add <hostname_or_ip>
```

**Removing a host:**
```bash
server-check --remove <hostname_or_ip>
```

**Running the dashboard:**
```bash
server-check
```

Press `Q` or `q` to gracefully exit the dashboard.
