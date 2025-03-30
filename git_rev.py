import subprocess
import os

try:
    # Get 0.0.0 version from latest Git tag
    tagcmd = "git describe --tags"
    version = subprocess.check_output(tagcmd, shell=True).decode().strip()
except subprocess.CalledProcessError:
    # W przypadku błędu z git, użyj domyślnej wartości
    version = "v0.0.0-dev"

print(f'\'-D FIRMWAREVERSION="{version}"\'')
