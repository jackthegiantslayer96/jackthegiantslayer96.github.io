import type { CSSProperties } from "react";
import type { Metadata } from "next";
import { Geist, Geist_Mono, Lora, IBM_Plex_Mono, Figtree } from "next/font/google";
import "./globals.css";
import { getSettings, type FontKey } from "@/lib/content";

const geistSans = Geist({ variable: "--font-geist-sans", subsets: ["latin"] });
const geistMono = Geist_Mono({ variable: "--font-geist-mono", subsets: ["latin"] });
const lora = Lora({ variable: "--font-lora", subsets: ["latin"] });
// IBM Plex Mono ships static weights only, so it needs one named explicitly.
const plexMono = IBM_Plex_Mono({ variable: "--font-plex-mono", subsets: ["latin"], weight: "400" });
const figtree = Figtree({ variable: "--font-figtree", subsets: ["latin"] });

// Font choices offered in the CMS settings collection. Each maps to a stack
// declared in globals.css, so switching one never needs a rebuild of the CSS.
const FONT_STACKS: Record<FontKey, string> = {
  inter: "var(--font-geist-sans), system-ui, sans-serif",
  serif: "var(--font-lora), Georgia, serif",
  mono: "var(--font-plex-mono), ui-monospace, monospace",
  rounded: "var(--font-figtree), var(--font-geist-sans), system-ui, sans-serif",
};

export async function generateMetadata(): Promise<Metadata> {
  const settings = await getSettings();
  return {
    title: {
      default: settings.siteName,
      template: `%s — ${settings.siteName}`,
    },
    description: settings.intro,
  };
}

// Runs before first paint so the correct theme is applied without a flash.
const themeScript = `(function(){try{var t=localStorage.getItem('theme');
var d=t?t==='dark':window.matchMedia('(prefers-color-scheme: dark)').matches;
if(d)document.documentElement.classList.add('dark');}catch(e){}})();`;

export default async function RootLayout({ children }: LayoutProps<"/">) {
  const settings = await getSettings();

  const themeVars = {
    // globals.css defines an --accent-<name> pair for every choice offered in
    // the CMS, so a bad value in settings.json cannot break the accent colour.
    "--accent": `var(--accent-${settings.accent})`,
    "--font-body": FONT_STACKS[settings.font],
  } as CSSProperties;

  return (
    <html
      lang="en"
      suppressHydrationWarning
      style={themeVars}
      className={`${geistSans.variable} ${geistMono.variable} ${lora.variable} ${plexMono.variable} ${figtree.variable} h-full antialiased`}
    >
      <head>
        <script dangerouslySetInnerHTML={{ __html: themeScript }} />
      </head>
      <body className="min-h-full flex flex-col bg-background text-foreground">
        {children}
      </body>
    </html>
  );
}