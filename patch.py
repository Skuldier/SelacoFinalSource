import websocket
ws = websocket.create_connection("ws://archipelago.gg:58697")
print("Success! It's a WebSocket server")
ws.close()