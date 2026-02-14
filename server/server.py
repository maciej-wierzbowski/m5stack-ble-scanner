#!/usr/bin/env python3
"""
Simple Flask server to receive BLE data from M5Stack Atom S3R

Requirements:
    pip install flask

Usage:
    python server.py

The server will listen on http://0.0.0.0:8080
"""

from flask import Flask, request, jsonify
from datetime import datetime
import json

app = Flask(__name__)

# Store received data (in production, use a database)
received_data = []

@app.route('/api/health', methods=['GET'])
def health_check():
    """Health check endpoint"""
    return jsonify({
        'status': 'ok',
        'timestamp': datetime.now().isoformat(),
        'devices_received': len(received_data)
    })

@app.route('/api/esp32/ble', methods=['POST'])
def receive_ble_data():
    """Receive BLE scan data from ESP32"""
    try:
        data = request.get_json()
        
        if not data:
            return jsonify({'error': 'No data provided'}), 400
        
        # Validate required fields
        if 'device_id' not in data or 'devices' not in data:
            return jsonify({'error': 'Missing required fields'}), 400
        
        # Add timestamp
        data['received_at'] = datetime.now().isoformat()
        
        # Store data
        received_data.append(data)
        
        # Print summary
        device_count = len(data['devices'])
        print(f"\n[{datetime.now().strftime('%H:%M:%S')}] Received from {data['device_id']}")
        print(f"  Devices: {device_count}")
        
        # Print device details
        for i, device in enumerate(data['devices'][:5], 1):  # First 5 devices
            name = device.get('name', 'Unknown')
            mac = device.get('mac', 'N/A')
            rssi = device.get('rssi', 'N/A')
            vendor = device.get('vendor', '')
            print(f"  {i}. {name:15} {mac:17} RSSI:{rssi:4} {vendor}")
        
        if device_count > 5:
            print(f"  ... and {device_count - 5} more devices")
        
        # Optionally save to file
        # with open('ble_data.jsonl', 'a') as f:
        #     f.write(json.dumps(data) + '\n')
        
        return jsonify({
            'status': 'ok',
            'received': device_count
        }), 200
        
    except Exception as e:
        print(f"Error processing request: {e}")
        return jsonify({'error': str(e)}), 500

@app.route('/api/data', methods=['GET'])
def get_data():
    """Get all received data"""
    return jsonify({
        'total_uploads': len(received_data),
        'data': received_data
    })

@app.route('/', methods=['GET'])
def index():
    """Simple status page"""
    return f"""
    <html>
    <head><title>BLE Scanner Server</title></head>
    <body>
        <h1>M5Stack BLE Scanner Server</h1>
        <p>Status: <strong style="color: green">Running</strong></p>
        <p>Uploads received: <strong>{len(received_data)}</strong></p>
        <h2>Endpoints:</h2>
        <ul>
            <li><a href="/api/health">/api/health</a> - Health check</li>
            <li><a href="/api/data">/api/data</a> - View received data</li>
            <li>POST /api/esp32/ble - Receive BLE data</li>
        </ul>
    </body>
    </html>
    """

if __name__ == '__main__':
    print("=" * 50)
    print("BLE Scanner Server Starting")
    print("=" * 50)
    print(f"Server will listen on: http://0.0.0.0:8080")
    print(f"Health check: http://localhost:8080/api/health")
    print(f"View data: http://localhost:8080/api/data")
    print("=" * 50)
    print()
    
    app.run(host='0.0.0.0', port=8080, debug=True)
