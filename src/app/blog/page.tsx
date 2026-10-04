import type { Metadata } from "next";
import BlogCard from "@/components/BlogCard";
import Footer from "@/components/Footer";
import Header from "@/components/Header";
import { getAllPosts } from "@/lib/content";

export const metadata: Metadata = {
  title: "Blog",
  description: "Notes on what I have been building.",
};

export default async function BlogPage() {
  const posts = await getAllPosts();

  return (
    <div className="mx-auto flex w-full max-w-3xl flex-1 flex-col px-6">
      <Header />

      <main className="flex-1 py-16 sm:py-24">
        <h1 className="text-4xl font-semibold tracking-tight sm:text-5xl">Blog</h1>
        <p className="mt-6 max-w-lg text-lg leading-8 text-muted">
          Notes on what I have been building.
        </p>

        {posts.length === 0 ? (
          <p className="mt-12 text-muted">No posts yet. Write one from the admin page.</p>
        ) : (
          <ul className="mt-12">
            {posts.map((post) => (
              <BlogCard key={post.slug} post={post} />
            ))}
          </ul>
        )}
      </main>

      <Footer />
    </div>
  );
}