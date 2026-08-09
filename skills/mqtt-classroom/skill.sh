#!/usr/bin/env bash
# mqtt-classroom skill - talk to the classroom MQTT broker from the shell.
#
# Requires the mosquitto clients (mosquitto_pub / mosquitto_sub):
#   Windows  installer from https://mosquitto.org/download/ (adds them to
#            C:\Program Files\mosquitto - add that folder to PATH)
#   macOS    brew install mosquitto
#   Linux    sudo apt install mosquitto-clients
#
# Override the broker without editing this file:
#   export MQTT_HOST=192.168.0.49
#   export MQTT_PORT=1883

set -euo pipefail

MQTT_HOST="${MQTT_HOST:-192.168.0.49}"
MQTT_PORT="${MQTT_PORT:-1883}"
BASE="classroom"

die() { echo "error: $*" >&2; exit 1; }

need_clients() {
  command -v mosquitto_sub >/dev/null 2>&1 || die "mosquitto_sub not found in PATH - see the header of this script"
  command -v mosquitto_pub >/dev/null 2>&1 || die "mosquitto_pub not found in PATH - see the header of this script"
}

usage() {
  cat <<EOF
mqtt-classroom - broker ${MQTT_HOST}:${MQTT_PORT}

  ./skill.sh check              test that the broker is reachable
  ./skill.sh devices            list boards that are online right now
  ./skill.sh watch [ID]         stream every message (or just one board's)
  ./skill.sh led ID on|off|toggle
  ./skill.sh sensor ID          stream that board's A0 readings
  ./skill.sh pub TOPIC PAYLOAD  publish anything (escape hatch)

ID is the board id printed on its serial monitor, e.g. c6-85ef58.

Topics
  ${BASE}/<id>/led/set      on | off | toggle      (you publish)
  ${BASE}/<id>/led/state    on | off               (retained)
  ${BASE}/<id>/sensor/a0    {"raw":2048,"mv":1650}
  ${BASE}/<id>/status       online | offline       (retained)
EOF
}

cmd_check() {
  need_clients
  local topic="${BASE}/_check/$$"
  echo "publishing to ${MQTT_HOST}:${MQTT_PORT} ..."
  if mosquitto_pub -h "$MQTT_HOST" -p "$MQTT_PORT" -t "$topic" -m "hello" 2>/dev/null; then
    echo "OK - broker reachable"
  else
    echo "FAILED - broker not reachable at ${MQTT_HOST}:${MQTT_PORT}" >&2
    echo "  - are you on the same WiFi as the broker machine?" >&2
    echo "  - is the broker's firewall allowing TCP ${MQTT_PORT}?" >&2
    exit 1
  fi
}

cmd_devices() {
  need_clients
  echo "boards reporting in (2s, Ctrl-C to stop early):"
  # Retained status messages arrive immediately on subscribe.
  mosquitto_sub -h "$MQTT_HOST" -p "$MQTT_PORT" -t "${BASE}/+/status" -v -W 2 2>/dev/null |
    while read -r topic payload; do
      id="${topic#${BASE}/}"; id="${id%/status}"
      printf '  %-12s %s\n' "$id" "$payload"
    done || true
}

cmd_watch() {
  need_clients
  local id="${1:-+}"
  echo "watching ${BASE}/${id}/# (Ctrl-C to stop)"
  mosquitto_sub -h "$MQTT_HOST" -p "$MQTT_PORT" -t "${BASE}/${id}/#" -v
}

cmd_led() {
  need_clients
  local id="${1:-}" state="${2:-}"
  [ -n "$id" ] && [ -n "$state" ] || die "usage: ./skill.sh led ID on|off|toggle"
  case "$state" in
    on|off|toggle) ;;
    *) die "state must be on, off or toggle" ;;
  esac
  mosquitto_pub -h "$MQTT_HOST" -p "$MQTT_PORT" -t "${BASE}/${id}/led/set" -m "$state"
  echo "sent '${state}' to ${id}"
}

cmd_sensor() {
  need_clients
  local id="${1:-}"
  [ -n "$id" ] || die "usage: ./skill.sh sensor ID"
  echo "streaming ${BASE}/${id}/sensor/a0 (Ctrl-C to stop)"
  mosquitto_sub -h "$MQTT_HOST" -p "$MQTT_PORT" -t "${BASE}/${id}/sensor/a0"
}

cmd_pub() {
  need_clients
  local topic="${1:-}" payload="${2:-}"
  [ -n "$topic" ] || die "usage: ./skill.sh pub TOPIC PAYLOAD"
  mosquitto_pub -h "$MQTT_HOST" -p "$MQTT_PORT" -t "$topic" -m "$payload"
  echo "published to ${topic}"
}

case "${1:-help}" in
  check)   shift; cmd_check "$@" ;;
  devices) shift; cmd_devices "$@" ;;
  watch)   shift; cmd_watch "$@" ;;
  led)     shift; cmd_led "$@" ;;
  sensor)  shift; cmd_sensor "$@" ;;
  pub)     shift; cmd_pub "$@" ;;
  help|-h|--help) usage ;;
  *) usage; exit 1 ;;
esac
