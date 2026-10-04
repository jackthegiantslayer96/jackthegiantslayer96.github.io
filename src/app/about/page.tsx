import type { Metadata } from "next";
import Footer from "@/components/Footer";
import Header from "@/components/Header";
import Prose from "@/components/Prose";
import { getAbout } from "@/lib/content";

export const metadata: Metadata = {
  title: "About",
  description: "A bit about me and how to get in touch.",
};

export default async function AboutPage() {
  const about = await getAbout();

  return (
    <div className="mx-auto flex w-full max-w-3xl flex-1 flex-col px-6">
      <Header />

      <main className="flex-1 py-16 sm:py-24">
        <h1 className="text-4xl font-semibold tracking-tight sm:text-5xl">About</h1>

        <div className="mt-10 flex flex-col gap-10 sm:flex-row sm:items-start">
          {about.photo && (
            <img
              src={about.photo}
              alt={about.name}
              className="h-32 w-32 shrink-0 rounded-full object-cover sm:h-40 sm:w-40"
            />
          )}

          <div className="min-w-0 flex-1">
            <p className="text-xl font-medium tracking-tight">{about.name}</p>
            {about.role && <p className="mt-1 text-muted">{about.role}</p>}

            {about.links.length > 0 && (
              <ul className="mt-6 flex flex-wrap gap-x-6 gap-y-2 text-sm">
                {about.links.map((link) => (
                  <li key={link.url}>
                    <a
                      href={link.url}
                      target={link.url.startsWith("mailto:") ? undefined : "_blank"}
                      rel="noreferrer noopener"
                      className="text-accent underline underline-offset-4 focus-visible:outline-2 focus-visible:outline-offset-4 focus-visible:outline-accent"
                    >
                      {link.label}
                    </a>
                  </li>
                ))}
              </ul>
            )}
          </div>
        </div>

        <Prose html={about.body} />
      </main>

      <Footer />
    </div>
  );
}