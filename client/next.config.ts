import type { NextConfig } from "next";
import { globals } from "./src/typescript/env";

const nextConfig: NextConfig = {
  /* config options here */
images: {
    remotePatterns: [
      {
        protocol: globals.url_string.scheme === 'http' ? 'http' : 'https',
        hostname: globals.url_string.subdomain,
        pathname: "/assets/**"
      },
    ],
    qualities: [1, 25, 75, 100]
  },
};

export default nextConfig;
