import Link from "next/link";
import ThemeToggle from "./ThemeToggle";

const NAV = [
  { href: "/projects", label: "Projects" },
  { href: "/apps", label: "Apps" },
  { href: "/blog", label: "Blog" },
  { href: "/about", label: "About" },
];

export default function Header() {
  return (
    <header className="flex items-center justify-between gap-4 py-6">
      <Link
        href="/"
        className="text-sm font-medium tracking-tight focus-visible:outline-2 focus-visible:outline-offset-4 focus-visible:outline-accent"
      >
        grammarhero
      </Link>
      <nav className="flex items-center gap-1 text-sm text-muted">
        {NAV.map((item) => (
          <Link
            key={item.href}
            href={item.href}
            className="rounded-md px-2 py-2 transition-colors hover:text-foreground focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-accent sm:px-3"
          >
            {item.label}
          </Link>
        ))}
        <a
          href="https://github.com"
          target="_blank"
          rel="noreferrer noopener"
          className="rounded-md px-2 py-2 transition-colors hover:text-foreground focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-accent sm:px-3"
        >
          GitHub
        </a>
        <ThemeToggle />
      </nav>
    </header>
  );
}