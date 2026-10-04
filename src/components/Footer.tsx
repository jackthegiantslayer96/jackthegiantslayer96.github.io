export default function Footer() {
  return (
    <footer className="mt-24 border-t border-foreground/10 py-8 text-sm text-muted">
      <p>
        Built with Next.js.{" "}
        <a
          href="https://github.com"
          target="_blank"
          rel="noreferrer noopener"
          className="text-foreground underline underline-offset-4 transition-colors hover:text-accent focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-accent"
        >
          Source
        </a>
      </p>
    </footer>
  );
}