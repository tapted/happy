#!/usr/bin/env python3
import argparse
import json
import os
import subprocess
import socket
import threading
import time
from http.server import SimpleHTTPRequestHandler, HTTPServer
import paho.mqtt.publish as publish

download_started = threading.Event()
download_finished = threading.Event()

class OtaRequestHandler(SimpleHTTPRequestHandler):
    def do_GET(self):
        print(f"[OTA] Serving to {self.client_address[0]}: {self.path}...")
        is_bin = self.path.endswith('.bin')
        if is_bin:
            print(f"\n[OTA] ESP32 requested {self.path}. Streaming to flash...", flush=True)
            download_started.set()
            
        # This call blocks while the file is transmitted over the socket
        super().do_GET() 
        
        if is_bin:
            print(f"\n[OTA] Transfer complete! ESP32 should now reboot.", flush=True)
            download_finished.set()

    def log_message(self, format, *args):
        print(f"[HTTP] {self.client_address[0]} - {format%args}", flush=True)

def get_local_ip():
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
        try:
            s.connect(('10.255.255.255', 1))
            return s.getsockname()[0]
        except Exception:
            return '127.0.0.1'

def get_git_hash(repo_dir):
    try:
        return subprocess.check_output(
            ['git', 'rev-parse', '--short', 'HEAD'], 
            cwd=repo_dir
        ).decode('utf-8').strip()
    except Exception:
        return "nogit"

def get_dynamic_version(repo_dir, base_version):
    state_file = os.path.join(repo_dir, '.ota_revision.json')
    current_hash = get_git_hash(repo_dir)
    rev = 1

    # Read existing state
    if os.path.exists(state_file):
        try:
            with open(state_file, 'r') as f:
                state = json.load(f)
            # If the git hash hasn't changed, increment the counter
            if state.get("hash") == current_hash:
                rev = state.get("rev", 0) + 1
        except Exception:
            pass # Reset to 1 on parse error

    # Save new state
    with open(state_file, 'w') as f:
        json.dump({"hash": current_hash, "rev": rev}, f)

    return f"{base_version}-{current_hash}-r{rev}"

def main():
    parser = argparse.ArgumentParser(description="HAPPY Firmware Local OTA Server")
    parser.add_argument("--bin", required=True, help="Path to the compiled .bin file")
    parser.add_argument("--project", required=True, help="Project name")
    parser.add_argument("--base-version", required=True, help="Base Firmware version (e.g., 1.0.0)")
    parser.add_argument("--broker", required=True, help="MQTT Broker IP")
    parser.add_argument("--topic", required=True, help="MQTT Trigger Topic")
    parser.add_argument("--user", required=False, help="MQTT Username", default=None)
    parser.add_argument("--password", required=False, help="MQTT Password", default=None)
    args = parser.parse_args()

    # We assume the script is run from the project root (where .git is)
    repo_dir = os.getcwd() 
    build_dir = os.path.dirname(os.path.abspath(args.bin))
    bin_filename = os.path.basename(args.bin)
    
    # Generate the dynamic version!
    dynamic_version = get_dynamic_version(repo_dir, args.base_version)
    
    manifest = {
        args.project: {
            "version": dynamic_version,
            "image": bin_filename
        }
    }
    
    manifest_path = os.path.join(build_dir, 'manifest.json')
    with open(manifest_path, 'w') as f:
        json.dump(manifest, f, indent=2)
    print(f"[OTA] Generated manifest.json -> Version: {dynamic_version}", flush=True)

    os.chdir(build_dir)
    server = HTTPServer(('0.0.0.0', 8032), OtaRequestHandler)
    server_thread = threading.Thread(target=server.serve_forever)
    server_thread.daemon = True
    server_thread.start()
    
    local_ip = get_local_ip()
    print(f"[OTA] Hosting firmware at http://{local_ip}:8032", flush=True)
    print(f"[OTA] Pushing MQTT trigger to {args.topic} on {args.broker}...", flush=True)
    
    time.sleep(0.5)
    try:
        auth_dict = None
        if args.user and args.password:
            auth_dict = {'username': args.user, 'password': args.password}
        publish.single(args.topic, payload="PRESS", hostname=args.broker, auth=auth_dict)
    except Exception as e:
        print(f"[OTA] Failed to publish MQTT message: {e}", flush=True)
        server.shutdown()
        return

    print("[OTA] Waiting for ESP32 to begin download...", flush=True)
    
    # Timeout 1: 30 seconds for the ESP32 to receive MQTT and ask for the binary
    if not download_started.wait(timeout=30.0):
        print("[OTA] Timeout! ESP32 did not request the binary. Check device logs.", flush=True)
        server.shutdown()
        return
        
    # Timeout 2: 300 seconds for the binary to finish streaming and flashing
    if not download_finished.wait(timeout=300.0):
        print("[OTA] Timeout! Transfer stalled or ESP32 crashed during flash.", flush=True)
    
    # Safe exit once the second event triggers
    server.shutdown()
    server_thread.join()

if __name__ == "__main__":
    main()