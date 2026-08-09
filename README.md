# mqtt-classroom

Shared MQTT setup for the XIAO ESP32C6 class: one Mosquitto broker, one firmware
sketch per board, and a skill students can clone to control the boards.

```
broker/     Mosquitto config that opens TCP 1883 + WebSockets 9001
firmware/   XIAO ESP32C6 sketch: LED control in, A0 readings out
skills/     mqtt-classroom skill (SKILL.md + skill.sh) for students
```

## For students

```bash
git clone <this repo>
cd mqtt-classroom/skills/mqtt-classroom
chmod +x skill.sh
./skill.sh check
./skill.sh devices
./skill.sh led c6-85ef58 on
```

Read [`skills/mqtt-classroom/SKILL.md`](skills/mqtt-classroom/SKILL.md) for the
topic map, prerequisites, and troubleshooting. You must be on the same WiFi as
the broker machine.

## For the instructor

### 1. Open the broker

Mosquitto 2.x binds to localhost only until a listener is declared, so a default
install accepts no LAN clients. In an **elevated** PowerShell:

```powershell
cd broker
powershell -ExecutionPolicy Bypass -File .\apply-broker-config.ps1
```

That backs up the existing config, installs `mosquitto.conf`, adds firewall
rules for 1883 and 9001 on the private profile, restarts the service, and prints
the LAN address to hand out.

### 2. Flash the boards

`firmware/MqttNode/` keeps credentials out of the repo:

```bash
cd firmware/MqttNode
cp arduino_secrets.example.h arduino_secrets.h   # then edit it
```

```bash
arduino-cli lib install PubSubClient
arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32C6 firmware/MqttNode
arduino-cli upload -p COM3 --fqbn esp32:esp32:XIAO_ESP32C6 firmware/MqttNode
```

Each board derives its own id from its MAC (`c6-` + last 3 bytes) and prints it
on the serial monitor at 115200 baud, so the same binary works on every board.

## Topics

| Topic | Direction | Payload |
|---|---|---|
| `classroom/<id>/led/set` | client → board | `on`, `off`, `toggle` |
| `classroom/<id>/led/state` | board → client | `on`, `off` (retained) |
| `classroom/<id>/sensor/a0` | board → client | `{"raw":2048,"mv":1650}` every 2s |
| `classroom/<id>/status` | board → client | `online`, `offline` (retained, last will) |

## Security

The broker runs with `allow_anonymous true` and no TLS. Anyone on the classroom
network can publish to any topic, including other students' boards. This is fine
for a lab on a trusted LAN and unacceptable anywhere else — do not expose port
1883 or 9001 to the internet.
