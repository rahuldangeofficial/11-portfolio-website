import React from 'react';

const Experience: React.FC = React.memo(() => {
  return (
    <section id="experience" className="space-y-6">
      <div className="pb-2 border-b border-[var(--border-default)]">
        <h2 className="text-xl sm:text-2xl font-bold tracking-tight text-[var(--fg-primary)]">
          Experience
        </h2>
      </div>

      <div className="space-y-4">
        <div className="space-y-1.5">
          <div className="flex flex-col sm:flex-row sm:items-baseline justify-between gap-1">
            <h3 className="text-base sm:text-lg font-bold text-[var(--fg-primary)]">
              Academic Tutor: Systems Programming (COMP2017)
            </h3>
            <span className="text-xs  text-[var(--fg-muted)]">
              Feb 2026 - Present
            </span>
          </div>

          <div className="flex flex-wrap items-center gap-x-3 text-xs sm:text-sm text-[var(--accent-cyan)] font-medium">
            <span>The University of Sydney</span>
            <span className="text-[var(--border-default)]">·</span>
            <span className="text-[var(--fg-muted)]">Sydney, Australia</span>
          </div>
        </div>

        <ul className="space-y-2 list-disc list-outside pl-4 text-sm sm:text-base leading-relaxed text-[var(--fg-secondary)]">
          <li>
            Instruct undergraduate computer science and software engineering students in C systems programming: memory layout, pointer arithmetic, struct alignment, and stack versus heap allocation.
          </li>
          <li>
            Conduct hands-on tutorials on POSIX multithreading and concurrency: mutexes, condition variables, semaphores, and race condition prevention.
          </li>
          <li>
            Guide debugging workflows for memory leaks, buffer overflows, and segmentation faults using GDB, Valgrind, and LLVM sanitizers.
          </li>
        </ul>

        <div className="flex flex-wrap gap-1.5 pt-2">
          {['C (C11)', 'POSIX Threads', 'Concurrency', 'Memory Layout', 'GDB', 'Valgrind', 'Sanitizers', 'Linux'].map((skill, idx) => (
            <span key={idx} className="skill-tag text-xs font-medium tracking-wide">
              {skill}
            </span>
          ))}
        </div>
      </div>
    </section>
  );
});

Experience.displayName = 'Experience';
export default Experience;
