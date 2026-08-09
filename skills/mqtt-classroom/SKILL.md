---
name: mqtt-classroom
description: Connect to the classroom Mosquitto broker to control XIAO ESP32C6 boards and read their A0 sensor values over MQTT. Use when the user wants to list boards, turn a board's LED on/off, watch sensor readings, or debug why they cannot reach the broker.
---

# mqtt-classroom

Every XIAO ESP32C6 in the class publishes to one shared Mosquitto broker. This
skill wraps the `mosquitto_pub` / `mosquitto_sub` commands behind `skill.sh`.

## Broker

| | |
|---|---|
| Host | `192.168.0.49` (the instructor's laptop) |
| MQTT (TCP) | `1883` — boards, CLI, desktop clients |
| MQTT over WebSockets | `9001` — browser clients |
| Auth | anonymous, no username or password |

You must be on the **same WiFi** as the broker machine. The address is a private
LAN address; it is not reachable from outside the classroom network.

Override the defaults with environment variables if the instructor's IP changes:

```bash
export MQTT_HOST=192.168.0.49
export MQTT_PORT=1883
```

## Topics

Each board owns a subtree keyed by its id (`c6-` + last 3 bytes of its MAC,
printed on the board's serial monitor at boot).

| Topic | Direction | Payload |
|---|---|---|
| `classroom/<id>/led/set` | you → board | `on`, `off`, `toggle` |
| `classroom/<id>/led/state` | board → you | `on`, `off` (retained) |
| `classroom/<id>/sensor/a0` | board → you | `{"raw":2048,"mv":1650}` every 2s |
| `classroom/<id>/status` | board → you | `online`, `offline` (retained, last will) |

`status` and `led/state` are retained, so a fresh subscriber learns the current
state immediately instead of waiting for the next change.

## Usage

```bash
./skill.sh check              # is the broker reachable at all?
./skill.sh devices            # which boards are online
./skill.sh led c6-85ef58 on   # LED on
./skill.sh led c6-85ef58 off
./skill.sh sensor c6-85ef58   # stream A0 readings
./skill.sh watch              # every message from every board
./skill.sh watch c6-85ef58    # one board only
```

## Prerequisites

`skill.sh` needs the mosquitto command line clients:

- **Windows** — installer from <https://mosquitto.org/download/>, then add
  `C:\Program Files\mosquitto` to `PATH`. Run `skill.sh` from Git Bash.
- **macOS** — `brew install mosquitto`
- **Linux** — `sudo apt install mosquitto-clients`

## Browser clients

For a web page, connect over WebSockets rather than TCP:

```js
// mqtt.js
const client = mqtt.connect('ws://192.168.0.49:9001')
client.subscribe('classroom/+/sensor/a0')
client.publish('classroom/c6-85ef58/led/set', 'on')
```

A browser cannot open a raw MQTT TCP socket, so port 1883 will not work from a
web page — that is what the 9001 listener exists for.

## Troubleshooting

| Symptom | Cause |
|---|---|
| `check` fails, connection refused | Broker not running, or still bound to localhost only — the instructor must apply `broker/mosquitto.conf` |
| `check` fails, timeout | Wrong WiFi, or the broker machine's firewall is blocking the port |
| `devices` prints nothing | No board is powered on, or the boards cannot reach the broker |
| Board shows `offline` | It lost WiFi or power; its last will fired |
| LED command accepted but nothing happens | Wrong board id — confirm with `./skill.sh devices` |
