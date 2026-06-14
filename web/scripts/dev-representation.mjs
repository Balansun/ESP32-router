#!/usr/bin/env node
/**
 * Start mock API (if needed) + Vite representation preview.
 * UI: http://127.0.0.1:5173/representation.html
 */
import { spawn } from "node:child_process";
import { existsSync } from "node:fs";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const webRoot = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const mockUrl = process.env.BALANSUN_MOCK_URL || "http://127.0.0.1:8787";
const healthUrl = `${mockUrl.replace(/\/$/, "")}/api/v1/health`;

async function mockHealthy() {
  try {
    const r = await fetch(healthUrl, { signal: AbortSignal.timeout(2000) });
    return r.ok;
  } catch {
    return false;
  }
}

function startMockNode() {
  const emulatorRoot = resolve(webRoot, "../../Balansun-Emulator");
  const server = resolve(emulatorRoot, "emulator/mock-server.mjs");
  if (!existsSync(server)) {
    console.error(
      "Mock API not reachable and Balansun-Emulator not found.\n" +
        "Start mock manually: docker compose -f Balansun-Emulator/docker-compose.mock.yml up -d\n" +
        "Or: node Balansun-Emulator/emulator/mock-server.mjs --port 8787",
    );
    process.exit(1);
  }
  const dataDir = resolve(emulatorRoot, "data");
  console.error(`Starting mock API → ${mockUrl}`);
  return spawn(
    process.execPath,
    [server, "--port", "8787", "--persist", resolve(dataDir, "device.json")],
    { cwd: resolve(emulatorRoot, "emulator"), stdio: "inherit" },
  );
}

async function waitForMock(maxMs = 15000) {
  const t0 = Date.now();
  while (Date.now() - t0 < maxMs) {
    if (await mockHealthy()) return true;
    await new Promise((r) => setTimeout(r, 400));
  }
  return false;
}

let mockChild = null;
if (!(await mockHealthy())) {
  mockChild = startMockNode();
  if (!(await waitForMock())) {
    mockChild?.kill("SIGTERM");
    console.error(`Mock API did not respond at ${healthUrl}`);
    process.exit(1);
  }
} else {
  console.error(`Mock API already up at ${mockUrl}`);
}

const vite = spawn("npm", ["run", "dev:representation:vite"], {
  cwd: webRoot,
  stdio: "inherit",
  shell: true,
  env: { ...process.env, BALANSUN_MOCK_URL: mockUrl },
});

function shutdown(code) {
  vite.kill("SIGTERM");
  mockChild?.kill("SIGTERM");
  process.exit(code ?? 0);
}

vite.on("exit", (code) => shutdown(code ?? 0));
process.on("SIGINT", () => shutdown(130));
process.on("SIGTERM", () => shutdown(143));
