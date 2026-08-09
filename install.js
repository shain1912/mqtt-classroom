#!/usr/bin/env node
/*
 * Installs the mqtt-classroom skill into ~/.claude/skills/mqtt-classroom.
 *
 *   npx github:shain1912/mqtt-classroom
 *
 * npx can run straight from the repo, so there is nothing to publish to npm.
 * Node only copies files here - no dependencies, no build step.
 */

const fs = require('fs');
const os = require('os');
const path = require('path');

const SKILL = 'mqtt-classroom';
const src = path.join(__dirname, 'skills', SKILL);
const dest = path.join(os.homedir(), '.claude', 'skills', SKILL);

function copyDir(from, to) {
  fs.mkdirSync(to, { recursive: true });
  for (const entry of fs.readdirSync(from, { withFileTypes: true })) {
    const a = path.join(from, entry.name);
    const b = path.join(to, entry.name);
    if (entry.isDirectory()) copyDir(a, b);
    else fs.copyFileSync(a, b);
  }
}

if (!fs.existsSync(src)) {
  console.error(`error: ${src} is missing - run this from a full checkout of the repo`);
  process.exit(1);
}

const existed = fs.existsSync(dest);
copyDir(src, dest);

// Windows ignores the mode bit; everywhere else the script must be runnable.
const sh = path.join(dest, 'skill.sh');
if (fs.existsSync(sh) && process.platform !== 'win32') fs.chmodSync(sh, 0o755);

console.log(`${existed ? 'Updated' : 'Installed'} skill: ${dest}`);
console.log('');
console.log('Next:');
console.log(`  cd ${dest}`);
console.log('  ./skill.sh name YOUR_NAME     # must match DEVICE_NAME in your board');
console.log('  ./skill.sh check              # is the broker reachable?');
console.log('  ./skill.sh devices            # which boards are online');
console.log('  ./skill.sh led on');
console.log('');
console.log('Needs the mosquitto clients (mosquitto_pub / mosquitto_sub):');
console.log('  Windows  https://mosquitto.org/download/   (default path is auto-detected)');
console.log('  macOS    brew install mosquitto');
console.log('  Linux    sudo apt install mosquitto-clients');
