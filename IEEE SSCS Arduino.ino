// Smart Pill Dispenser with Doctor/Patient Login - Full Arduino Code
// Hardware: ESP32, 4 Servos, PIR Sensor

#include <WiFi.h>
#include <ESP32Servo.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <time.h>

const char* ssid = "Chanakya";
const char* password = "9163974212";
const int trigPin = 22;  // D22
const int echoPin = 23;  // D23
const int servoPins[4] = {19, 18, 14, 27};

Servo servos[4];
WebServer server(80);

const int stopPWM = 93;
const int ccwPWM = 85;
const int cwPWM = 110;
const int wiggleDelay = 150;
const int betweenPillDelay = 1500;

unsigned long lastNTPUpdate = 0;
const unsigned long ntpUpdateInterval = 2UL * 60UL * 60UL * 1000UL;

int dispenseHour = 8;
int dispenseMinute = 0;
int pillsToDispense[4];
bool dispensing = false;
bool wasDispensing = false; 

String doctorUser = "doctor";
String doctorPass = "1234";
String patientUser = "patient";
String patientPass = "5678";
bool isDoctor = false;
bool isLoggedIn = false;

String dispenseHistory = "<tr><th>Date</th><th>Time</th><th>Pills (Slot 1-4)</th></tr>";

String formatTime(int hour, int minute) {
  String period = "AM";
  if (hour >= 12) period = "PM";
  int displayHour = hour % 12;
  if (displayHour == 0) displayHour = 12;
  char buf[6];
  sprintf(buf, "%02d:%02d", displayHour, minute);
  return String(buf) + " " + period;
}

void setup() {
  Serial.begin(115200);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  EEPROM.begin(100);
  loadSettings();


  for (int i = 0; i < 4; i++) {
    servos[i].setPeriodHertz(50);
    servos[i].attach(servoPins[i], 500, 2500);
    servos[i].write(stopPWM);
  }

  connectToWiFi();
  syncTime();
  setupServer();
}

void loop() {
  server.handleClient();
  if (millis() - lastNTPUpdate > ntpUpdateInterval) syncTime();

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return;

  if (timeinfo.tm_hour == dispenseHour && timeinfo.tm_min == dispenseMinute && !dispensing) {
    dispensing = true;
    Serial.println("Starting pill dispensing");
    for (int i = 0; i < 4; i++) dispensePills(i, pillsToDispense[i]);

    char dateBuffer[20];
    strftime(dateBuffer, sizeof(dateBuffer), "%Y-%m-%d", &timeinfo);
    String formattedTime = formatTime(timeinfo.tm_hour, timeinfo.tm_min);
    dispenseHistory += "<tr><td>" + String(dateBuffer) + "</td><td>" + formattedTime + "</td><td>" + String(pillsToDispense[0]) + ", " + String(pillsToDispense[1]) + ", " + String(pillsToDispense[2]) + ", " + String(pillsToDispense[3]) + "</td></tr>";

    Serial.println("Finished dispensing");
    dispensing = false;
    delay(60000);
  }
  delay(500);
}

void dispensePills(int servoIndex, int pillCount) {
  int dispensed = 0;
  bool detectionInProgress = false;
  unsigned long lastTriggerTime = 0;
  unsigned long lastDetectionTime = 0; // new: for cooldown
    const unsigned long detectionCooldown = 250; // 1 second

  while (dispensed < pillCount) {
    // Start motor
    servos[servoIndex].write(ccwPWM);

    // Ultrasonic measurement every 20ms max
    if (millis() - lastTriggerTime >= 6.5) {
      lastTriggerTime = millis();

      // Trigger ultrasonic pulse
      digitalWrite(trigPin, LOW);
      delayMicroseconds(2);
      digitalWrite(trigPin, HIGH);
      delayMicroseconds(10);
      digitalWrite(trigPin, LOW);

      // Measure echo time
      long duration = pulseIn(echoPin, HIGH, 30000);
      float distance = duration * 0.034 / 2.0;

      Serial.print("Distance: ");
      Serial.print(distance, 3);
      Serial.print(" cm");

  if (distance < 2.295 || distance > 2.313) {
  if (millis() - lastDetectionTime >= detectionCooldown) {
    dispensed++;
    lastDetectionTime = millis();  // update last detection timestamp
    Serial.println(" Pill Detected — Total: " + String(dispensed));

    servos[servoIndex].write(stopPWM);
    delay(300);  // Pause motor briefly
  } else {
    Serial.println("Skipped duplicate detection");
  }
  } else {
  Serial.println(" Not in detection zone");
  }

    }
  }

  servos[servoIndex].write(stopPWM);
  Serial.println("✅ Servo " + String(servoIndex + 1) + " finished dispensing " + String(pillCount) + " pills");
}


void connectToWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected. IP: " + WiFi.localIP().toString());
}

void syncTime() {
  configTime(-7 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) lastNTPUpdate = millis();
}
setInterval(() => {
  fetch("/status")
    .then(res => res.json())
    .then(data => {
      if (wasDispensing && data.dispensing === false) {
        // Dispensing just finished
        const sound = document.getElementById("ding");
        if (sound) sound.play().catch(() => {});
      }
      wasDispensing = data.dispensing;
    })
void loadSettings() {
  for (int i = 0; i < 4; i++) pillsToDispense[i] = EEPROM.read(i);
  dispenseHour = EEPROM.read(4);
  dispenseMinute = EEPROM.read(5);
}

void saveSettings() {
  for (int i = 0; i < 4; i++) EEPROM.write(i, pillsToDispense[i]);
  EEPROM.write(4, dispenseHour);
  EEPROM.write(5, dispenseMinute);
  EEPROM.commit();
}

String loginPage() {
  return R"rawliteral(
    <html><head><meta name='viewport' content='width=device-width, initial-scale=1'>
    <style>
      body{font-family:sans-serif;text-align:center;background:#ffc3a0;margin:0;padding:0;}
      input{padding:10px;width:80%;margin-top:10px;}
      button{padding:10px;width:85%;background:#ef476f;color:white;border:none;border-radius:5px;}
      @media (max-width: 600px){input,button{width:95%;}}
    </style></head><body>
    <h2>Login</h2>
    <form action='/login' method='POST'>
      <input name='username' placeholder='Username'><br>
      <input name='password' type='password' placeholder='Password'><br>
      <button type='submit'>Login</button>
    </form></body></html>
  )rawliteral";
}

String doctorDashboard() {
  String html = R"rawliteral(
    <html><head><meta name='viewport' content='width=device-width, initial-scale=1'>
    <style>
      body{font-family:Arial;background:linear-gradient(to right,#43cea2,#185a9d);color:white;padding:20px;}
      input,button{padding:8px;width:100%;margin-top:10px;border:none;border-radius:5px;}
      table{width:100%;border-collapse:collapse;margin-top:20px;}
      th,td{padding:10px;border:1px solid white;text-align:center;}
      button{background:#f7971e;color:white;}
      @media(max-width:600px){input,button{width:95%;}}
    </style></head><body>
    <h2>Doctor Dashboard</h2>
    <form action='/set' method='GET'>
  )rawliteral";

  for (int i = 0; i < 4; i++) {
    html += "Pill Slot " + String(i+1) + " Count: <input name='s" + String(i) + "' type='number'><br>";
  }

  html += "<label>Dispense Time:</label><br><input name='hour' type='number' placeholder='Hour (0-23)'> : <input name='minute' type='number' placeholder='Minute (0-59)'><br>";
  html += "<button type='submit'>Save Settings</button></form><br><br>";
  html += "<h3>Dispense History</h3><table id='historyTable'>" + dispenseHistory + "</table>";
  html += "<br><button onclick=\"exportTableToCSV('pill_history.csv')\">Export History</button>";

  html += R"rawliteral(
    <script>
    function exportTableToCSV(filename) {
      var csv = [];
      var rows = document.querySelectorAll("table tr");
      for (var i = 0; i < rows.length; i++) {
        var row = [], cols = rows[i].querySelectorAll("td, th");
        for (var j = 0; j < cols.length; j++) row.push(cols[j].innerText);
        csv.push(row.join(","));
      }
      var csvFile = new Blob([csv.join("\n")], { type: "text/csv" });
      var downloadLink = document.createElement("a");
      downloadLink.download = filename;
      downloadLink.href = window.URL.createObjectURL(csvFile);
      downloadLink.style.display = "none";
      document.body.appendChild(downloadLink);
      downloadLink.click();
    }
    </script>
  )rawliteral";

  html += "<br><form action='/logout' method='GET'><button type='submit'>Logout</button></form></body></html>";
  return html;
}

String patientDashboard() {
  String html = R"rawliteral(
    <html><head><meta name='viewport' content='width=device-width, initial-scale=1'>
    <style>
      body{font-family:Arial;background:linear-gradient(to right,#8360c3,#2ebf91);color:white;padding:20px;}
      table{width:100%;border-collapse:collapse;margin-top:20px;}
      th,td{padding:10px;border:1px solid white;text-align:center;}
      button{background:#f7971e;color:white;padding:8px;width:100%;margin-top:10px;border:none;border-radius:5px;}
      @media(max-width:600px){body{font-size:14px;}}
    </style></head><body> <audio id="ding" src="https://www.soundjay.com/buttons/sounds/button-3.mp3" preload="auto"></audio>
    <h2>Patient Dashboard</h2>
  )rawliteral";


  html += "<p>Next Dispense Time: " + formatTime(dispenseHour, dispenseMinute) + "</p>";
  for (int i = 0; i < 4; i++) {
    html += "<p>Pill Slot " + String(i+1) + ": " + String(pillsToDispense[i]) + " pills</p>";
  }

  html += "<h3>Dispense History</h3><table id='historyTable'>" + dispenseHistory + "</table>";
  html += "<br><button onclick=\"exportTableToCSV('pill_history.csv')\">Export History</button>";

  html += R"rawliteral(
    <script>
    function exportTableToCSV(filename) {
      var csv = [];
      var rows = document.querySelectorAll("table tr");
      for (var i = 0; i < rows.length; i++) {
        var row = [], cols = rows[i].querySelectorAll("td, th");
        for (var j = 0; j < cols.length; j++) row.push(cols[j].innerText);
        csv.push(row.join(","));
      }
      var csvFile = new Blob([csv.join("\n")], { type: "text/csv" });
      var downloadLink = document.createElement("a");
      downloadLink.download = filename;
      downloadLink.href = window.URL.createObjectURL(csvFile);
      downloadLink.style.display = "none";
      document.body.appendChild(downloadLink);
      downloadLink.click();
    }
    let wasDispensing = false;

setInterval(() => {
  fetch("/status")
    .then(res => res.json())
    .then(data => {
      if (wasDispensing && data.dispensing === false) {
        // Dispensing just finished
        const sound = document.getElementById("ding");
        if (sound) sound.play().catch(() => {});
      }
      wasDispensing = data.dispensing;
    })
    .catch(err => console.error("Status check failed", err));
}, 3000); // Poll every 3 seconds

    </script>
  )rawliteral";

  html += "<br><form action='/logout' method='GET'><button type='submit'>Logout</button></form></body></html>";
  return html;
}


void setupServer() {
  server.on("/", HTTP_GET, []() {
    if (!isLoggedIn) server.send(200, "text/html", loginPage());
    else if (isDoctor) server.send(200, "text/html", doctorDashboard());
    else server.send(200, "text/html", patientDashboard());
  });

  server.on("/login", HTTP_POST, []() {
    String user = server.arg("username");
    String pass = server.arg("password");
    if ((user == doctorUser && pass == doctorPass) || (user == patientUser && pass == patientPass)) {
      isLoggedIn = true;
      isDoctor = (user == doctorUser);
      server.sendHeader("Location", "/");
      server.send(303);
    } else {
      server.send(401, "text/html", "<h3>Invalid credentials</h3><a href='/'>Try Again</a>");
    }
  });

  server.on("/logout", HTTP_GET, []() {
    isLoggedIn = false;
    isDoctor = false;
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.on("/set", HTTP_GET, []() {
    for (int i = 0; i < 4; i++) {
      if (server.hasArg("s" + String(i))) pillsToDispense[i] = constrain(server.arg("s" + String(i)).toInt(), 0, 10);
    }
    if (server.hasArg("hour")) dispenseHour = constrain(server.arg("hour").toInt(), 0, 23);
    if (server.hasArg("minute")) dispenseMinute = constrain(server.arg("minute").toInt(), 0, 59);
    saveSettings();
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.on("/status", HTTP_GET, []() {
  String json = "{\"dispensing\": ";
  json += dispensing ? "true" : "false";
  json += "}";
  server.send(200, "application/json", json);
});


  server.begin();
  Serial.println("Web Server Started");
}
