import Link from "next/link";
import type { BlogPost } from "@/lib/content";

function formatDate(date: string) {
  const parsed = new Date(date);
  if (Number.isNaN(parsed.getTime())) return date;
  return parsed.toLocaleDateString("en-GB", { year: "numeric", month: "short", day: "numeric" });
}

export default function BlogCard({ post }: { post: BlogPost }) {
  return (
    <li>
      <Link
        href={`/blog/${post.slug}`}
        className="group flex flex-col gap-3 border-t border-foreground/10 py-6 transition-colors focus-visible:outline-2 focus-visible:outline-offset-4 focus-visible:outline-accent sm:flex-row sm:items-baseline sm:gap-6"
      >
        <span className="font-mono text-xs text-muted tabular-nums sm:w-28 sm:shrink-0">
          {formatDate(post.date)}
        </span>
        <span className="flex-1">
          <span className="block text-lg font-medium tracking-tight transition-colors group-hover:text-accent">
            {post.title}
          </span>
          <span className="mt-1 block text-sm text-muted">{post.description}</span>
        </span>
        <span aria-hidden="true" className="text-muted transition-transform group-hover:translate-x-0.5">
          →
        </span>
      </Link>
    </li>
  );
}