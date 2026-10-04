import type { App } from "@/lib/content";

export default function AppCard({ app }: { app: App }) {
  return (
    <li className="group relative flex flex-col gap-3 rounded-xl border border-foreground/10 p-6 transition-colors hover:border-accent/40 focus-within:border-accent/40">
      <div className="flex items-center gap-3">
        {app.icon ? (
          <img
            src={app.icon}
            alt=""
            aria-hidden="true"
            className="h-9 w-9 rounded-lg object-cover"
          />
        ) : (
          <span
            aria-hidden="true"
            className="flex h-9 w-9 items-center justify-center rounded-lg bg-accent/10 text-sm font-medium text-accent"
          >
            {app.title.slice(0, 2).toUpperCase()}
          </span>
        )}
        <h2 className="text-lg font-medium tracking-tight">{app.title}</h2>
      </div>

      <p className="flex-1 text-sm leading-6 text-muted">{app.description}</p>

      <a
        href={app.url}
        target="_blank"
        rel="noreferrer noopener"
        className="text-sm font-medium text-accent underline underline-offset-4 focus-visible:outline-2 focus-visible:outline-offset-4 focus-visible:outline-accent"
      >
        Open app
        <span aria-hidden="true"> →</span>
      </a>
    </li>
  );
}