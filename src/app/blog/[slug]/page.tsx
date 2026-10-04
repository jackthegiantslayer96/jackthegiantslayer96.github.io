import type { Metadata } from "next";
import Link from "next/link";
import { notFound } from "next/navigation";
import Footer from "@/components/Footer";
import Header from "@/components/Header";
import Prose from "@/components/Prose";
import { getPostBySlug, getPostSlugs } from "@/lib/content";

// Required for static export: every slug must be known at build time.
export async function generateStaticParams() {
  const slugs = await getPostSlugs();
  return slugs.map((slug: string) => ({ slug }));
}

export async function generateMetadata({
  params,
}: PageProps<"/blog/[slug]">): Promise<Metadata> {
  const { slug } = await params;
  const post = await getPostBySlug(slug);
  return { title: post?.title ?? "Post", description: post?.description };
}

export default async function PostPage({ params }: PageProps<"/blog/[slug]">) {
  const { slug } = await params;
  const post = await getPostBySlug(slug);

  if (!post) notFound();

  return (
    <div className="mx-auto flex w-full max-w-3xl flex-1 flex-col px-6">
      <Header />

      <main className="flex-1 py-16 sm:py-24">
        <Link
          href="/blog"
          className="text-sm text-muted transition-colors hover:text-foreground focus-visible:outline-2 focus-visible:outline-offset-4 focus-visible:outline-accent"
        >
          &larr; All posts
        </Link>

        <h1 className="mt-6 text-4xl font-semibold tracking-tight sm:text-5xl">
          {post.title}
        </h1>

        <time dateTime={post.date} className="mt-4 block font-mono text-sm text-muted">
          {post.date}
        </time>

        {post.cover && (
          <img
            src={post.cover}
            alt=""
            className="mt-10 aspect-video w-full rounded-xl border border-foreground/10 object-cover"
          />
        )}

        <Prose html={post.body} />
      </main>

      <Footer />
    </div>
  );
}