import type { NextConfig } from "next";

// GitHub Pages serves user sites from https://<user>.github.io/ and project
// sites from https://<user>.github.io/<repo>/. This repo is a user site, so
// basePath stays "". For a project site, set both to "/<repo-name>" — if
// assetPrefix drifts from basePath, every CSS and JS chunk 404s.
const basePath = "";

const nextConfig: NextConfig = {
  output: "export",
  basePath,
  assetPrefix: basePath || undefined,
  // Emits /about/index.html instead of /about.html so deep links resolve
  // without a server doing URL rewriting.
  trailingSlash: true,
  // No image optimizer exists in a static build.
  images: { unoptimized: true },
};

export default nextConfig;
