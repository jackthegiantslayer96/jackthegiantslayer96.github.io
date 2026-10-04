import Link from "next/link";
import Footer from "@/components/Footer";
import Header from "@/components/Header";
import ProjectCard from "@/components/ProjectCard";
import { getAllProjects, getSettings } from "@/lib/content";

export default async function Home() {
  const [settings, projects] = await Promise.all([getSettings(), getAllProjects()]);

  const featured = settings.featuredProjects.length > 0
    ? projects.filter((project) => settings.featuredProjects.includes(project.slug))
    : projects.slice(0, 3);

  return (
    <div className="mx-auto flex w-full max-w-3xl flex-1 flex-col px-6">
      <Header />

      <main className="flex-1">
        <section className="py-16 sm:py-24">
          <h1 className="max-w-xl text-4xl font-semibold tracking-tight sm:text-5xl">
            {settings.tagline}
          </h1>
          <p className="mt-6 max-w-lg text-lg leading-8 text-muted">{settings.intro}</p>

          <div className="mt-10 flex flex-wrap gap-x-6 gap-y-2 text-sm">
            <Link
              href="/projects"
              className="text-accent underline underline-offset-4 focus-visible:outline-2 focus-visible:outline-offset-4 focus-visible:outline-accent"
            >
              Projects
            </Link>
            <Link
              href="/apps"
              className="text-accent underline underline-offset-4 focus-visible:outline-2 focus-visible:outline-offset-4 focus-visible:outline-accent"
            >
              Apps
            </Link>
            <Link
              href="/blog"
              className="text-accent underline underline-offset-4 focus-visible:outline-2 focus-visible:outline-offset-4 focus-visible:outline-accent"
            >
              Blog
            </Link>
            <Link
              href="/about"
              className="text-accent underline underline-offset-4 focus-visible:outline-2 focus-visible:outline-offset-4 focus-visible:outline-accent"
            >
              About
            </Link>
          </div>
        </section>

        <section id="work" className="scroll-mt-8">
          <h2 className="text-sm font-medium uppercase tracking-widest text-muted">
            Selected work
          </h2>

          {featured.length === 0 ? (
            <p className="mt-8 text-muted">
              No projects yet. Add one from the admin page.
            </p>
          ) : (
            <ul className="mt-4">
              {featured.map((project, index) => (
                <ProjectCard key={project.slug} project={project} index={index} />
              ))}
            </ul>
          )}
        </section>
      </main>

      <Footer />
    </div>
  );
}