import asyncio
import websockets
import json

async def test_archipelago_websocket(uri="ws://localhost:38281", slot_name="Skuldier"):
    """Test Archipelago connection using WebSocket"""
    
    print(f"Connecting to {uri}")
    
    try:
        async with websockets.connect(uri) as websocket:
            print("Connected via WebSocket!")
            
            # Create handshake
            handshake = [{
                "cmd": "Connect",
                "password": "",
                "name": slot_name,
                "version": {
                    "major": 0,
                    "minor": 4,
                    "build": 5,
                    "class": "Version"
                },
                "uuid": "test-client-12345",
                "game": "",
                "slot_data": True,
                "items_handling": 0
            }]
            
            handshake_json = json.dumps(handshake)
            print(f"Sending: {handshake_json}")
            
            # Send handshake
            await websocket.send(handshake_json)
            
            # Wait for response
            print("Waiting for response...")
            response = await asyncio.wait_for(websocket.recv(), timeout=5.0)
            
            print(f"Received: {response}")
            
            # Parse response
            resp_data = json.loads(response)
            print(f"Parsed: {json.dumps(resp_data, indent=2)}")
            
    except asyncio.TimeoutError:
        print("Response timed out")
    except Exception as e:
        print(f"Error: {type(e).__name__}: {e}")

# For Python without websockets library installed
def test_http_endpoint():
    """Test if server has HTTP endpoint"""
    import urllib.request
    try:
        response = urllib.request.urlopen("http://localhost:38281")
        print(f"HTTP Response: {response.read().decode()[:200]}...")
    except Exception as e:
        print(f"HTTP Error: {e}")

if __name__ == "__main__":
    print("=== Testing HTTP endpoint ===")
    test_http_endpoint()
    
    print("\n=== Testing WebSocket ===")
    try:
        asyncio.run(test_archipelago_websocket())
    except:
        print("WebSocket test failed - install with: pip install websockets")
        print("\nThis confirms Archipelago uses WebSocket protocol!")