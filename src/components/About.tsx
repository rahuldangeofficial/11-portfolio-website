import React from 'react';

const About: React.FC = React.memo(() => {
  return (
    <section id="about" className="space-y-6">
      <div className="pb-2 border-b border-[var(--border-default)]">
        <h2 className="text-xl sm:text-2xl font-bold tracking-tight text-[var(--fg-primary)]">
          About
        </h2>
      </div>

      <div className="space-y-4 text-sm sm:text-base leading-relaxed text-[var(--fg-secondary)]">
        <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
          <div className="p-4 rounded-lg border border-[var(--border-default)] bg-[var(--bg-primary)] space-y-1.5">
            <h3 className="text-sm font-bold text-[var(--fg-primary)]">
              Systems Programming
            </h3>
            <p className="text-sm sm:text-base text-[var(--fg-secondary)] leading-relaxed">
              C11/C++ with POSIX threads, atomic operations, custom memory allocators, and sanitizer-verified concurrency. Built <strong className="text-[var(--fg-primary)] ">MIMFS</strong> and <strong className="text-[var(--fg-primary)] ">LLFPL</strong> from scratch.
            </p>
          </div>

          <div className="p-4 rounded-lg border border-[var(--border-default)] bg-[var(--bg-primary)] space-y-1.5">
            <h3 className="text-sm font-bold text-[var(--fg-primary)]">
              Full-Stack Development
            </h3>
            <p className="text-sm sm:text-base text-[var(--fg-secondary)] leading-relaxed">
              JavaScript, Node.js, Express, MongoDB, and React. Built REST APIs, real-time apps with Socket.io, and interactive frontends.
            </p>
          </div>
        </div>

      </div>
    </section>
  );
});

About.displayName = 'About';
export default About;