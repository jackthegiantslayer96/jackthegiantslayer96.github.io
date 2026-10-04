import type { Metadata } from "next";
import AppCard from "@/components/AppCard";
import Footer from "@/components/Footer";
import Header from "@/components/Header";
import { getAllApps } from "@/lib/content";

export const metadata: Metadata = {
  title: "Apps",
  description: "Web apps and tools I have built and host.",
};

export default async function AppsPage() {
  const apps = await getAllApps();

  return (
    <div className="mx-auto flex w-full max-w-3xl flex-1 flex-col px-6">
      <Header />

      <main className="flex-1 py-16 sm:py-24">
        <h1 className="text-4xl font-semibold tracking-tight sm:text-5xl">Apps</h1>
        <p className="mt-6 max-w-lg text-lg leading-8 text-muted">
          Small tools and web apps I have built and host. Each one opens in its
          own tab.
        </p>

        {apps.length === 0 ? (
          <p className="mt-12 text-muted">No apps yet. Add one from the admin page.</p>
        ) : (
          <ul className="mt-12 grid gap-4 sm:grid-cols-2">
            {apps.map((app) => (
              <AppCard key={app.slug} app={app} />
            ))}
          </ul>
        )}
      </main>

      <Footer />
    </div>
  );
}