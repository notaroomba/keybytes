// Starts the app for development.
//
// On macOS, an Electron started straight from a terminal is treated as part of the terminal
// app for privacy checks. The first Bluetooth scan is then charged to Terminal / iTerm /
// VS Code, which has no Bluetooth usage string, and macOS kills the app on the spot.
// Starting it through LaunchServices (`open`) makes Electron.app itself responsible, and
// its Info.plist does declare Bluetooth use.
const { spawn, execSync } = require('child_process');
const path = require('path');

const appDir = path.join(__dirname, '..');
const electron = require('electron'); // path to the binary
const args = process.argv.slice(2);

if (process.platform !== 'darwin' || process.env.KEYWORD_DIRECT_START) {
  const child = spawn(electron, [appDir, ...args], { stdio: 'inherit' });
  child.on('exit', (code) => process.exit(code ?? 0));
} else {
  const bundle = path.resolve(path.dirname(electron), '..', '..'); // …/Electron.app
  // Send the app's output back to this terminal, since `open` would otherwise drop it.
  let tty = null;
  try { tty = execSync('tty', { stdio: ['inherit', 'pipe', 'ignore'] }).toString().trim(); } catch {}
  const io = tty && tty.startsWith('/dev/') ? ['--stdout', tty, '--stderr', tty] : [];
  const child = spawn('open', ['-W', '-n', ...io, bundle, '--args', appDir, ...args], { stdio: 'inherit' });
  child.on('exit', (code) => process.exit(code ?? 0));
}
