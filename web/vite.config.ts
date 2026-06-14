import { defineConfig, loadEnv } from "vite";
import { resolve } from "node:path";
import { fileURLToPath } from "node:url";
import { viteSingleFile } from "vite-plugin-singlefile";

const webRoot = fileURLToPath(new URL(".", import.meta.url));

export default defineConfig(({ mode }) => {
  const env = loadEnv(mode, process.cwd(), "");
  const fwVersion = env.VITE_FIRMWARE_VERSION || "dev";
  const representationDev = mode === "representation";
  const mockApiTarget = env.BALANSUN_MOCK_URL || "http://127.0.0.1:8787";

  return {
    // Firmware serves the SPA at GET / (same base in dev and production).
    base: "/",
    plugins: [
      viteSingleFile(),
      {
        name: "balansun-pwa-html",
        transformIndexHtml(html) {
          let out = html.replace(/%VITE_FIRMWARE_VERSION%/g, fwVersion);
          out = out.replace(
            /<!-- BALANSUN_PWA_HEAD -->[\s\S]*?<!-- \/BALANSUN_PWA_HEAD -->/,
            [
              "<!-- BALANSUN_PWA_HEAD -->",
              `<meta name="balansun-ui-version" content="${fwVersion}" />`,
              '<meta name="apple-mobile-web-app-capable" content="yes" />',
              '<meta name="apple-mobile-web-app-status-bar-style" content="black-translucent" />',
              '<meta name="apple-mobile-web-app-title" content="Balansun" />',
              "<!-- /BALANSUN_PWA_HEAD -->",
            ].join("\n    "),
          );
          return out;
        },
      },
    ],
    build: {
      sourcemap: false,
      target: "es2020",
      cssCodeSplit: false,
      assetsInlineLimit: 100_000_000,
      minify: "terser",
      terserOptions: {
        compress: {
          passes: 3,
          drop_console: true,
          drop_debugger: true,
          pure_getters: true,
        },
        mangle: { toplevel: true },
        format: { comments: false },
      },
      rollupOptions: {
        output: {
          inlineDynamicImports: true,
          manualChunks: undefined,
        },
      },
      reportCompressedSize: true,
    },
    server: {
      port: 5173,
      strictPort: false,
      host: true,
      open: representationDev ? "/representation.html" : false,
      proxy: representationDev
        ? {
            "/api": { target: mockApiTarget, changeOrigin: true },
          }
        : undefined,
    },
    ...(representationDev
      ? {
          appType: "spa" as const,
          build: {
            rollupOptions: {
              input: {
                main: resolve(webRoot, "index.html"),
                representation: resolve(webRoot, "representation.html"),
              },
            },
          },
        }
      : {}),
  };
});
