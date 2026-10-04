import type { Metadata } from "next";
import Link from "next/link";
import { notFound } from "next/navigation";
import Footer from "@/components/Footer";
import Header from "@/components/Header";
import Prose from "@/components/Prose";
import { getProjectBySlug, getProjectSlugs } from "@/lib/content";

// Required for static export: every slug must be known at build time.
export async function generateStaticParams() {
  const slugs = await getProjectSlugs();
  return slugs.map((slug) => ({ slug }));
}

export async function generateMetadata({
  params,
}: PageProps<"/projects/[slug]">): Promise<Metadata> {
  const { slug } = await params;
  const project = await getProjectBySlug(slug);
  return { title: project?.title ?? "Project", description: project?.summary };
}

export default async function ProjectPage({
  params,
}: PageProps<"/projects/[slug]">) {
  const { slug } = await params;
  const project = await getProjectBySlug(slug);

  if (!project) notFound();

  return (
    <div className="mx-auto flex w-full max-w-3xl flex-1 flex-col px-6">
      <Header />

      <main className="flex-1 py-16 sm:py-24">
        <Link
          href="/#work"
          className="text-sm text-muted transition-colors hover:text-foreground focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-accent"
        >
          &larr; All work
        </Link>

        <h1 className="mt-6 text-4xl font-semibold tracking-tight sm:text-5xl">
          {project.title}
        </h1>

        <div className="mt-4 flex flex-wrap items-center gap-3 text-sm text-muted">
          <span className="font-mono tabular-nums">{project.year}</span>
          {project.stack.map((tech) => (
            <span
              key={tech}
              className="rounded-full border border-foreground/15 px-2.5 py-0.5"
            >
              {tech}
            </span>
          ))}
        </div>

        {project.image && (
          <img
            src={project.image}
            alt=""
            className="mt-10 aspect-video w-full rounded-xl border border-foreground/10 object-cover"
          />
        )}

        <Prose html={project.body} />

        {project.href && (
          <a
            href={project.href}
            target="_blank"
            rel="noreferrer noopener"
            className="mt-10 inline-flex items-center gap-2 text-sm font-medium text-accent underline underline-offset-4 focus-visible:outline-2 focus-visible:outline-offset-4 focus-visible:outline-accent"
          >
            Visit project
            <span aria-hidden="true">&rarr;</span>
          </a>
        )}
      </main>

      <Footer />
    </div>
  );
}
