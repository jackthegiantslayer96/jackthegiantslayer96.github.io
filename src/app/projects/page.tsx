import type { Metadata } from "next";
import Footer from "@/components/Footer";
import Header from "@/components/Header";
import ProjectCard from "@/components/ProjectCard";
import { getAllProjects } from "@/lib/content";

export const metadata: Metadata = {
  title: "Projects",
  description: "Things I have designed, built, and shipped.",
};

export default async function ProjectsPage() {
  const projects = await getAllProjects();

  return (
    <div className="mx-auto flex w-full max-w-3xl flex-1 flex-col px-6">
      <Header />

      <main className="flex-1 py-16 sm:py-24">
        <h1 className="text-4xl font-semibold tracking-tight sm:text-5xl">Projects</h1>

        {projects.length === 0 ? (
          <p className="mt-12 text-muted">
            No projects yet. Add one from the admin page.
          </p>
        ) : (
          <ul className="mt-12">
            {projects.map((project, index) => (
              <ProjectCard key={project.slug} project={project} index={index} />
            ))}
          </ul>
        )}
      </main>

      <Footer />
    </div>
  );
}