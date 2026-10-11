-- scripts/server.lua

-- Handle standard secure page serving
function route_http(path)
    if path == "/" then
        return [[
            <html>
            <head><title>Anomalon Core</title></head>
            <body>
                <h1>Welcome to Anomalon Web Engine</h1>
                <p>Status: ]] .. Engine.status .. [[</p>
                <div id="updates">Awaiting stream updates...</div>

                <script>
                    // Simple WebSocket code connecting right back over SSL
                    const ws = new WebSocket("wss://" + window.location.host);
                    ws.onmessage = (event) => {
                        document.getElementById("updates").innerText = event.data;
                    };
                    setInterval(() => ws.send("request_update"), 2000);
                </script>
            </body>
            </html>
        ]]
    elseif path == "/health" then
        return "{ 'status': '" .. Engine.status .. "' }"
    else
        return "Page Not Found"
    end
end

-- Handle high-frequency WebSocket messaging
function on_websocket_message(payload)
    if payload == "request_update" then
        -- Return raw text or JSON representing live element updates
        return "Dynamic Metric: Server timestamp is " .. os.time()
    end
    return "Unknown instruction"
end

