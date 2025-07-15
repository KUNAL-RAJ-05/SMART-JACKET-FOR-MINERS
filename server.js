const express = require("express");
const http = require("http");
const fs = require("fs");
const { SerialPort } = require("serialport");
const { ReadlineParser } = require("@serialport/parser-readline");
const socketIo = require("socket.io");
const app = express();
const server = http.createServer(app);
const io = socketIo(server);

app.use(express.static("public"));
app.use(express.json());

// In-memory session tracking
let currentSession = null;
let sessionLogs = [];

// Load saved logs
const logFilePath = "logs.json";
try {
  const content = fs.readFileSync(logFilePath, "utf8");
  sessionLogs = content.trim() ? JSON.parse(content) : [];
} catch (err) {
  sessionLogs = [];
}

app.get("/api/logs", (req, res) => {
  res.json(sessionLogs);
});

const port = new SerialPort({ path: "COM4", baudRate: 115200 });
const parser = port.pipe(new ReadlineParser({ delimiter: "\n" }));

parser.on("data", (line) => {
  const cleanLine = line.replace(/^.*?->\s*/, "").trim();

  // Sensor data
  if (cleanLine.includes("Temprature:") && cleanLine.includes("Pulse:")) {
    const data = {};
    cleanLine.split("-").forEach(part => {
      const [key, value] = part.split(":");
      const k = key.trim().toLowerCase();
      if (k.includes("temp")) data["Temperature"] = parseFloat(value);
      else if (k.includes("pulse")) data["Pulse"] = parseInt(value);
      else if (k.includes("gas")) data["Gas"] = parseInt(value);
      else if (k.includes("bt")) data["BT connected"] = parseInt(value);
    });
    io.emit("sensorData", data);
    return;
  }

  // Handle session events
  io.emit("sessionMessage", cleanLine);

  if (cleanLine.startsWith("Hello,")) {
    const name = cleanLine.split(",")[1].trim();
    currentSession = {
      name,
      startTime: new Date().toLocaleTimeString(),
      endTime: null,
      duration: null
    };
  }

  if (cleanLine.startsWith("Goodbye")) {
    if (currentSession) {
      const parts = cleanLine.split("Duration:");
      currentSession.endTime = new Date().toLocaleTimeString();
      currentSession.duration = parts[1].trim();
      sessionLogs.push(currentSession);
      fs.writeFileSync(logFilePath, JSON.stringify(sessionLogs, null, 2));
      currentSession = null;
    }
  }
});

server.listen(3000, () => {
  console.log("✅ Server running at http://localhost:3000");
});
