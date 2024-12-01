import os
import json

Import("env")

# File containing board-specific flags
flags_file = "upesy_wroom.json"

# Ensure the file exists
if not os.path.exists(flags_file):
    raise FileNotFoundError(f"{flags_file} not found!")

# Load the JSON data
with open(flags_file, "r") as file:
    flags = json.load(file)

# Get the current environment's board
env_board = env["BOARD"]

# Apply flags if the board is defined in the JSON
if env_board in flags:
    env.Append(CPPDEFINES=flags[env_board]["build_flags"])
else:
    print(f"No custom flags defined for {env_board}.")