const YOUTUBE_HOSTS = ["youtube.com", "www.youtube.com", "m.youtube.com", "youtu.be"];
const VIMEO_HOSTS = ["vimeo.com", "www.vimeo.com", "player.vimeo.com"];

// Returns the embeddable URL for a YouTube or Vimeo link, or undefined for
// anything else so callers can fall back to rendering a normal link.
export function getVideoEmbedUrl(url: string): string | undefined {
  let parsed: URL;
  try {
    parsed = new URL(url.trim());
  } catch {
    return undefined;
  }

  const host = parsed.hostname.toLowerCase();

  if (YOUTUBE_HOSTS.includes(host)) {
    const id =
      host === "youtu.be"
        ? parsed.pathname.slice(1)
        : (parsed.searchParams.get("v") ?? parsed.pathname.split("/").pop());
    return id ? `https://www.youtube-nocookie.com/embed/${id}` : undefined;
  }

  if (VIMEO_HOSTS.includes(host)) {
    const id = parsed.pathname.split("/").filter(Boolean).pop();
    return id ? `https://player.vimeo.com/video/${id}` : undefined;
  }

  return undefined;
}