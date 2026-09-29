import React from 'react';

const additionalExperience = [
  {
    title: 'Systems Engineer & Chief Technology Officer',
    company: 'Alethi',
    employment: 'Self-employed',
    dates: 'Jan 2026 - Aug 2026',
    location: 'Sydney, New South Wales, Australia · Hybrid',
    description: 'Equity-based role building MCK1, a unikernel architecture that uses a high-level language and graph description to build custom kernels and runtimes. Designed the low-level system architecture for direct hardware execution, with a focus on security, performance and deterministic behaviour.',
  },
  {
    title: 'Full-Stack Engineer & Chief Technology Officer',
    company: 'ChessVerst',
    employment: 'Self-employed',
    dates: 'Feb 2021 - Jul 2023',
    location: 'Pune District, Maharashtra, India · Hybrid',
    description: 'Equity-based role designing the platform architecture from wireframes through frontend, backend and database design. Built the platform using React, Next.js, PostgreSQL and REST APIs, including reusable frontend and backend components.',
  },
  {
    title: 'Associate Software Engineer',
    company: 'Lyte Calc Technologies Pvt Ltd',
    employment: 'Internship',
    dates: 'Mar 2022 - Aug 2022',
    location: 'Bengaluru, Karnataka, India · Remote',
    description: 'Built an e-commerce platform using React, Express.js, Node.js and MongoDB with a reusable application architecture. Developed frontend components, backend services and REST APIs, and integrated them across the application.',
  },
];

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
      {additionalExperience.map(role => (
        <div className="space-y-4" key={role.company}>
          <div className="space-y-1.5">
            <div className="flex flex-col sm:flex-row sm:items-baseline justify-between gap-1">
              <h3 className="text-base sm:text-lg font-bold text-[var(--fg-primary)]">{role.title}</h3>
              <span className="text-xs text-[var(--fg-muted)] shrink-0">{role.dates}</span>
            </div>
            <div className="flex flex-wrap items-center gap-x-3 text-xs sm:text-sm text-[var(--accent-cyan)] font-medium">
              <span>{role.company} · {role.employment}</span>
              <span className="text-[var(--border-default)]">·</span>
              <span className="text-[var(--fg-muted)]">{role.location}</span>
            </div>
          </div>
          <ul className="space-y-2 list-disc list-outside pl-4 text-sm sm:text-base leading-relaxed text-[var(--fg-secondary)]">
            <li>{role.description}</li>
          </ul>
        </div>
      ))}
    </section>
  );
});

Experience.displayName = 'Experience';
export default Experience;
