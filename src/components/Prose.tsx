// Renders HTML produced by the Markdown pipeline in src/lib/content.ts, which
// is sanitized against an allow-list before it reaches this component.
export default function Prose({ html }: { html: string }) {
  return (
    <div
      className="prose-grammarhero max-w-none"
      dangerouslySetInnerHTML={{ __html: html }}
    />
  );
}