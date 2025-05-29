# Rootkit

A simple and educational rootkit designed for modern Linux kernels.  
**Tested on:** Ubuntu 22.04, Kernel version 6.8.0 and above.
> **Note:** This project is under active development and is not ready yet.

## Features

- **Network Backdoor**  
  Filters network packets for magic payloads to trigger commands or actions.
- **SO Injector**  
  Receives a shared object over the network and injects it into a target process.
- **Binary Loader**  
  Executes ELF binaries received via the network.
- **Keylogger**  
  Captures and logs keyboard input.
- **Persistence**  
  Implements basic methods to survive reboots.
- **Evasion Techniques**  
  Utilizes fundamental strategies to avoid detection.

## ⚠️ Disclaimer

> **Warning:** This project is intended for educational and research purposes only.  
> Unauthorized deployment or use of rootkits is illegal and unethical.

## General Description

This rootkit operates as a kernel-level backdoor. It hooks into netfilter to inspect all packets (currently focusing on UDP) for predefined "magic" payloads. If such a payload is found, the rootkit drops the packet before it reaches userspace and executes a corresponding command.  
Available commands include the features listed above, and the rootkit is designed to be modular: it can receive and inject shared objects (SO) or execute binaries received over the network, allowing for extensive extensibility even after initial installation.

The repository includes example userspace helpers for:
- Spawning a reverse shell
- Setting up persistence
- Retrieving keystroke logs from the kernel-level keylogger

## Usage

- Example install and uninstall scripts (`install.sh` and `uninstall.sh`) demonstrate how to deploy the rootkit, using innocuous file names for stealth.
- A controller script (`rootkitctl.py`, written in Python 3) is provided to run on the attacker’s machine and interact with the rootkit’s backdoor functionality.
