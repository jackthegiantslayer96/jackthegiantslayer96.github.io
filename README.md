# grammarhero

Static portfolio site built with Next.js (App Router) and Tailwind CSS v4, deployed to GitHub Pages. Content is edited through a web admin page at `/admin`.

## Commands

```bash
npm run dev     # start the dev server
npm run build   # static export to ./out
npm run lint    # eslint
```

## Editing the site

Go to **`/admin`**, sign in with GitHub, and edit text boxes or drop in images. Saving commits the change to this repository, GitHub Actions rebuilds, and the live site updates in about a minute. No terminal required.

Collections available in the admin:

| Section | What it edits |
|---|---|
| Projects | `src/content/projects/*.md` — the project list and detail pages |
| Apps | `src/content/apps/*.md` — the Apps page |
| Blog posts | `src/content/blog/*.md` — the blog list and post pages |
| Site settings | `src/content/settings.json` — site name, tagline, accent colour, font |
| About page | `src/content/about.md` — name, role, photo, links, bio |

### Images and video

- **Images** are uploaded through the admin and stored in `public/uploads/`. Use the image widget rather than pasting a link.
- **Video** is not uploaded. Paste a YouTube or Vimeo URL on its own line in the body of a project or post and it becomes an embedded player automatically (using `youtube-nocookie.com`). Hosting video files here would slow the site down and eat the free GitHub Pages bandwidth.

### Colours and fonts

Two layers:

- The **Site settings** page in the admin has an accent-colour picker (blue, teal, violet, amber, rose, lime) and a font picker (sans, serif, mono, rounded). Only these options are accepted, so a bad value cannot break the styling.
- For anything the picker does not offer, edit `src/app/globals.css`. Each choice maps to a custom property there — `--accent-<name>` for colours and `--font-body` for the body font — so you can add new ones by adding a variable and a matching option in `public/admin/config.yml`.

The theme (light/dark) is applied by a blocking inline script in `layout.tsx` so it doesn't flash white on load. Keep that script in `<head>`.

## Content model

```
src/content/
  settings.json      # site name, tagline, intro, accent, font, featured projects
  about.md           # frontmatter: name, role, photo, links
  projects/<slug>.md
  apps/<slug>.md
  blog/<slug>.md
```

Markdown bodies are rendered with `marked` and passed through `sanitize-html` with an allow-list, so pasted HTML cannot inject scripts. `generateStaticParams` builds a page per slug at build time — a new file means a new page automatically.

## Deployment

The repo must be named **`<your-github-username>.github.io`** for a user site. `basePath` in `next.config.ts` is `""` for that layout, which serves the site at the domain root.

If you rename the repo to anything else, it becomes a project site served from `github.io/<repo-name>`. In that case set both `basePath` and `assetPrefix` to `"/<repo-name>"` — if they drift apart, every CSS and JS chunk 404s.

Pushes to `main` or `master` run `.github/workflows/deploy.yml`, which builds and publishes to GitHub Pages. In the repo settings, set **Pages → Source** to **GitHub Actions**.

## Notes

- `output: "export"` means no server. Server Actions, cookies, rewrites, and the default image optimizer are unavailable. `images.unoptimized` is set for this reason, and `<img>` is used instead of `next/image`.
- `trailingSlash: true` emits `about/index.html` rather than `about.html` so deep links resolve on a static host without URL rewriting.
- The CMS backend in `public/admin/config.yml` has `repo` and `branch` that must match this repository, or saving an entry will fail.