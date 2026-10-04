import Link from "next/link";
import type { Project } from "@/lib/content";

export default function ProjectCard({
  project,
  index,
}: {
  project: Project;
  index: number;
}) {
  return (
    <li>
      <Link
        href={`/projects/${project.slug}`}
        className="group flex flex-col gap-2 border-t border-foreground/10 py-6 transition-colors focus-visible:outline-2 focus-visible:outline-offset-4 focus-visible:outline-accent sm:flex-row sm:items-baseline sm:gap-6"
      >
        <span className="font-mono text-xs text-muted tabular-nums">
          {String(index + 1).padStart(2, "0")}
        </span>
        <span className="flex-1">
          <span className="block text-lg font-medium tracking-tight transition-colors group-hover:text-accent">
            {project.title}
          </span>
          <span className="mt-1 block text-sm text-muted">{project.summary}</span>
        </span>
        <span className="font-mono text-xs text-muted tabular-nums">
          {project.year}
        </span>
      </Link>
    </li>
  );
}
