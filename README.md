# IIS-Reverse-Shell

## Overview
This project demonstrates advanced penetration testing techniques against IIS web servers, focusing on reverse shell deployment, privilege escalation, and persistence mechanisms. It includes:

- **Ansible Playbooks**: Automate the deployment and configuration of IIS servers across multiple targets
- **PHP Scripts**: Facilitate reverse shell functionality and SYSTEM privilege escalation
- **Website Content**: Professional-looking HTML and assets for social engineering
- **Persistence Mechanisms**: C++ Windows service for maintaining long-term access
- **Mass RCE Capabilities**: Python server for coordinated command execution across multiple compromised hosts

## Repository Structure

<pre>IIS-Reverse-Shell/
├── Ansible/
│   ├── iis_setup.yml
│   ├── inventory.yml
│   └── ansible.cfg
├── Images/
│   ├── UB_Lockdown/
│   ├── IRSEC/
│   └── Empire State Health/
├── PHP/
│   ├── contact.php
│   ├── search.php
│   └── php.ini
├── Persistence/
│   ├── Main.cpp
│   ├── Service.h
│   ├── Service.cpp
│   ├── Persistence.h
│   ├── Persistence.cpp
│   └── IISManagerService.exe
├── Website/
│   ├── UB_Lockdown/
│   │   ├── UB_Lockdown.html
│   │   ├── button.js
│   │   └── web.config
│   ├── IRSEC/
│   │   ├── IRSEC.html
│   │   ├── button.js
│   │   └── web.config
│   └── Empire State Health/
│       ├── Empire State Health.html
│       ├── button.js
│       └── web.config
└── server.py
</pre>

### Directory Breakdown
- **`Ansible/`**: Contains playbooks and inventory files for automating server setup.
- **`Images/`**: Directory with images used in the website.
- **`PHP/`**: Contains the PHP script for the reverse shell.
- **`Persistence/`**: Contains the C++ executable service that maintains persistence on the server.
- **`Website/`**: HTML and CSS files for the website's frontend.
- **`server.py`**: A Python Client that allows for mass remote command execution.

## Setup Instructions

### Prerequisites
- **Ansible**: Installed on the control machine.
- **Windows Server**: With WinRM running and enabled.
- **Network Configuration**: Ensure the server is accessible and that required ports are properly configured.
- **Required Packages**:
  ```sh
  sudo apt install git
  sudo apt install software-properties-common
  sudo add-apt-repository ppa:ansible/ansible --yes --update
  sudo apt install ansible
  sudo apt install python3-pip
  pip install inquirerpy
  ```

### Steps

## 1. Clone the Repository
```bash
cd ~
git clone https://github.com/Lcdemi/IIS-Reverse-Shell
cd IIS-Reverse-Shell/Ansible
```

## 2. Configure Ansible Inventory
Edit the inventory.yml file to include your server's details:

```yaml
all:
  hosts:
  children:
    winserver:
      hosts:
        [IP_1]:
        [IP_2]:
        ...
    win10:
      hosts:
        [IP_1]:
        [IP_2]:
        ...
  vars:
    ansible_user: your_username
    ansible_password: your_password
    ansible_connection: winrm
    ansible_port: 5985
    ansible_winrm_transport: ntlm
    ansible_winrm_server_cert_validation: ignore
```

## 3. Configure Competition Name
Edit the iis_setup.yml file to include the name of the Competition:

```yaml
 name: IIS Reverse Shell Server Configuration
  hosts: all
  gather_facts: true
  vars:
    Competition: "[Your Competition Name]"
```

## 4. Run the Ansible Playbook
To set up the IIS server and deploy the website, execute the following command:

```sh
ansible-playbook -i inventory.yml iis_setup.yml -f 50
```

## 5. Remote Code Execution
For remote command execution and reverse shell deployment, use the included `server.py` Python tool.

### Features
- **Singular Command Execution**: Run commands on individual targets
- **Mass Command Execution**: Execute commands across multiple targets simultaneously
- **Reverse Shell Deployment**: Spawn reverse shells to your listener
- **Target Group Management**: Pre-defined groups for different host types
- **Real-time Results**: Live output from all connected systems

### Usage

#### Starting the Tool
```sh
python3 server.py
```

#### Available Actions
- **Singular Remote Code Execution**: Execute commands on single targets
- **Mass Remote Code Execution**: Run commands across target groups
- **Spawn a Reverse Shell**: Deploy reverse shells

#### Target Groups
- **All Hosts**
- **Domain Controllers**
- **IIS Hosts**
- **WinRM Hosts**
- **ICMP Hosts**
- **SMB Hosts**
- **Custom Targets (Manual IP input)**

Note: The target IP addresses in server.py are pre-configured for a specific network environment (192.168.X.X and 10.X.1.X ranges). You may need to modify the ALL_HOSTS, ALL_DC, ALL_IIS, ALL_WINRM, ALL_ICMP, and ALL_SMB arrays in the script to match your target network configuration.

#### Example Usage
```bash
# Check privileges across all IIS servers
> Select "Mass Remote Code Execution"
> Choose "All IIS Hosts"
> Enter command: whoami /priv

# Deploy reverse shell to specific target
> Select "Spawn a Reverse Shell"
> Enter target: 192.168.4.3
> Enter listener: 10.65.0.10:6767
```
