import { getVideoEmbedUrl } from "@/lib/video";

export default function Video({ url, title }: { url: string; title?: string }) {
  const embedUrl = getVideoEmbedUrl(url);

  if (!embedUrl) {
    return (
      <a
        href={url}
        target="_blank"
        rel="noreferrer noopener"
        className="text-accent underline underline-offset-4"
      >
        Watch the video
      </a>
    );
  }

  return (
    <div className="aspect-video w-full overflow-hidden rounded-lg border border-foreground/10 bg-foreground/5">
      <iframe
        src={embedUrl}
        title={title ?? "Embedded video"}
        loading="lazy"
        allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture"
        allowFullScreen
        className="h-full w-full"
      />
    </div>
  );
}