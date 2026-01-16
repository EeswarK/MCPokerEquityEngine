#!/bin/bash
# Quick script to verify WebSocket binary packets
# Usage: ./verify_websocket_binary.sh <websocket_url>
#
# Options:
# 1. Using websocat (install: cargo install websocat)
# 2. Using wscat (install: npm install -g wscat)
# 3. Using Python script

WS_URL="${1:-ws://localhost:8000/ws/test-job-id}"

echo "Verifying binary packets on: $WS_URL"
echo ""
echo "Option 1: Using websocat (shows binary as hex)"
echo "  websocat --binary $WS_URL | xxd"
echo ""
echo "Option 2: Using wscat (shows binary as base64)"
echo "  wscat -c $WS_URL"
echo ""
echo "Option 3: Using Python (decode and display)"
python3 << EOF
import asyncio
import websockets
import sys

async def listen():
    uri = "$WS_URL"
    async with websockets.connect(uri) as websocket:
        print(f"Connected to {uri}")
        print("Waiting for binary messages...")
        try:
            while True:
                message = await websocket.recv()
                if isinstance(message, bytes):
                    print(f"Received binary packet: {len(message)} bytes")
                    print(f"Hex: {message.hex()[:100]}...")
                else:
                    print(f"Received text: {message}")
        except websockets.exceptions.ConnectionClosed:
            print("Connection closed")

asyncio.run(listen())
EOF
