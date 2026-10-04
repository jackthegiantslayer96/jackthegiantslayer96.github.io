import { promises as fs } from "node:fs";
import path from "node:path";
import matter from "gray-matter";
import { marked } from "marked";
import sanitizeHtml from "sanitize-html";
import { getVideoEmbedUrl } from "@/lib/video";

export type Project = {
  slug: string;
  title: string;
  year: string;
  summary: string;
  stack: string[];
  href?: string;
  image?: string;
  body: string;
};

export type App = {
  slug: string;
  title: string;
  description: string;
  url: string;
  icon?: string;
  embed: boolean;
};

export type BlogPost = {
  slug: string;
  title: string;
  date: string;
  description: string;
  cover?: string;
  body: string;
};

export type About = {
  name: string;
  role: string;
  photo?: string;
  links: { label: string; url: string }[];
  body: string;
};

export type AccentKey = "blue" | "teal" | "violet" | "amber" | "rose" | "lime";

export type FontKey = "inter" | "serif" | "mono" | "rounded";

export type Settings = {
  siteName: string;
  tagline: string;
  intro: string;
  accent: AccentKey;
  font: FontKey;
  featuredProjects: string[];
};

const CONTENT_DIR = path.join(process.cwd(), "src", "content");

const ACCENTS: AccentKey[] = ["blue", "teal", "violet", "amber", "rose", "lime"];
const FONTS: FontKey[] = ["inter", "serif", "mono", "rounded"];

// Content files are authored in the CMS by a single trusted editor, but the
// rendered HTML is injected with dangerouslySetInnerHTML, so the allow-list
// keeps a stray script or iframe from a pasted embed out of the page.
const ALLOWED_TAGS = [
  "p", "br", "strong", "em", "del", "code", "pre", "blockquote",
  "h2", "h3", "h4", "ul", "ol", "li", "a", "img", "hr",
  "figure", "figcaption", "table", "thead", "tbody", "tr", "th", "td",
];

// A YouTube or Vimeo link sitting alone on a line is turned into an embed, so
// pasting a video URL in the CMS body is all that is needed to add one.
const bareVideoLink =
  /<p>\s*<a[^>]*href="([^"]+)"[^>]*>\s*\1\s*<\/a>\s*<\/p>/g;

const markdownToHtml = (markdown: string): string => {
  const html = marked.parse(markdown, { async: false, gfm: true }) as string;

  return sanitizeHtml(html, {
    allowedTags: [...ALLOWED_TAGS, "iframe", "div"],
    allowedAttributes: {
      a: ["href", "title", "target", "rel"],
      img: ["src", "alt", "title", "width", "height", "loading"],
      iframe: ["src", "title", "loading", "allow", "allowfullscreen", "frameborder"],
      div: ["class"],
    },
    allowedSchemes: ["http", "https", "mailto"],
    allowedSchemesByTag: { iframe: ["https"] },
    transformTags: {
      a: sanitizeHtml.simpleTransform("a", { rel: "noreferrer noopener" }, true),
    },
  }).replace(bareVideoLink, (match: string, rawUrl: string) => {
    let url = rawUrl;
    try {
      url = decodeURIComponent(rawUrl);
    } catch {
      // A malformed escape sequence is not worth failing the build over; the
      // link simply stays as a normal anchor.
    }

    const embedUrl = getVideoEmbedUrl(url);
    if (!embedUrl) return match;

    return `<div class="aspect-video my-6"><iframe src="${embedUrl}" title="Embedded video" loading="lazy" allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture" allowfullscreen frameborder="0"></iframe></div>`;
  });
};

async function readMarkdownFile(filePath: string) {
  const raw = await fs.readFile(filePath, "utf8");
  const { data, content } = matter(raw);
  return { data, body: content.trim() };
}

async function readCollection(
  folder: string,
): Promise<Array<{ slug: string } & Record<string, unknown>>> {
  const dir = path.join(CONTENT_DIR, folder);
  let files: string[];
  try {
    files = (await fs.readdir(dir)).filter((file) => file.endsWith(".md"));
  } catch {
    return [];
  }

  return Promise.all(
    files.sort().map(async (file) => {
      const slug = file.replace(/\.md$/, "");
      const { data, body } = await readMarkdownFile(path.join(dir, file));
      return { slug, ...data, body };
    }),
  );
}

const asString = (value: unknown, fallback = ""): string =>
  typeof value === "string" ? value : fallback;

const asStringList = (value: unknown): string[] =>
  Array.isArray(value) ? value.filter((item): item is string => typeof item === "string") : [];

const asBoolean = (value: unknown): boolean => value === true || value === "true";

export async function getAllProjects(): Promise<Project[]> {
  const entries = await readCollection("projects");
  return entries
    .map((entry) => ({
      slug: entry.slug,
      title: asString(entry.title),
      year: asString(entry.year),
      summary: asString(entry.summary),
      stack: asStringList(entry.stack),
      href: asString(entry.href) || undefined,
      image: asString(entry.image) || undefined,
      body: markdownToHtml(asString(entry.body)),
    }))
    .sort((a, b) => b.year.localeCompare(a.year));
}

export async function getProjectBySlug(slug: string): Promise<Project | undefined> {
  const projects = await getAllProjects();
  return projects.find((project) => project.slug === slug);
}

export async function getProjectSlugs(): Promise<string[]> {
  const projects = await getAllProjects();
  return projects.map((project) => project.slug);
}

export async function getAllApps(): Promise<App[]> {
  const entries = await readCollection("apps");
  return entries.map((entry) => ({
    slug: entry.slug,
    title: asString(entry.title),
    description: asString(entry.description),
    url: asString(entry.url),
    icon: asString(entry.icon) || undefined,
    embed: asBoolean(entry.embed),
  }));
}

export async function getAllPosts(): Promise<BlogPost[]> {
  const entries = await readCollection("blog");
  return entries
    .map((entry) => ({
      slug: entry.slug,
      title: asString(entry.title),
      date: asString(entry.date),
      description: asString(entry.description),
      cover: asString(entry.cover) || undefined,
      body: markdownToHtml(asString(entry.body)),
    }))
    .sort((a, b) => b.date.localeCompare(a.date));
}

export async function getPostBySlug(slug: string): Promise<BlogPost | undefined> {
  const posts = await getAllPosts();
  return posts.find((post) => post.slug === slug);
}

export async function getPostSlugs(): Promise<string[]> {
  const posts = await getAllPosts();
  return posts.map((post) => post.slug);
}

export async function getAbout(): Promise<About> {
  try {
    const { data, body } = await readMarkdownFile(path.join(CONTENT_DIR, "about.md"));
    return {
      name: asString(data.name),
      role: asString(data.role),
      photo: asString(data.photo) || undefined,
      links: Array.isArray(data.links)
        ? (data.links as { label?: unknown; url?: unknown }[]).map((link) => ({
            label: asString(link.label),
            url: asString(link.url),
          }))
        : [],
      body: markdownToHtml(body),
    };
  } catch {
    return { name: "", role: "", links: [], body: "" };
  }
}

export async function getSettings(): Promise<Settings> {
  let parsed: Record<string, unknown> = {};
  try {
    parsed = JSON.parse(await fs.readFile(path.join(CONTENT_DIR, "settings.json"), "utf8"));
  } catch {
    // Falls through to the defaults below when the file is absent or invalid.
  }

  const accent = asString(parsed.accent) as AccentKey;
  const font = asString(parsed.font) as FontKey;

  return {
    siteName: asString(parsed.siteName, "grammarhero"),
    tagline: asString(parsed.tagline),
    intro: asString(parsed.intro),
    accent: ACCENTS.includes(accent) ? accent : "blue",
    font: FONTS.includes(font) ? font : "inter",
    featuredProjects: asStringList(parsed.featuredProjects),
  };
}

export { markdownToHtml };