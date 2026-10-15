import React from 'react';

const experience = [
  {
    title: 'Academic Tutor',
    company: 'The University of Sydney',
    employment: 'Part-time',
    dates: 'Jan 2026 - Present',
    location: 'Camperdown, New South Wales, Australia · On-site',
    description: 'Delivered tutorials and supported students learning C and systems programming, including memory management, pointers, processes, inter-process communication and concurrency. Helped students reason about program behaviour and debug their implementations using tools including GDB and Valgrind.',
  },
  {
    title: 'System Engineer',
    company: 'Alethi',
    employment: 'Part-time',
    dates: 'Jan 2026 - Aug 2026',
    location: 'Sydney, New South Wales, Australia · Hybrid',
    description: 'Worked on a project basis on System One’s MCK1 kernel/compiler initiative, contributing low-level software development and systems architecture. Conducted exploratory photonic integrated-circuit research, developing experimental concepts and prototype models. This work remained exploratory and was not fully validated.',
  },
  {
    title: 'Project Lead & Full-Stack Developer (Industry Capstone)',
    company: 'University of Sydney × Aeoflo',
    employment: '',
    dates: 'Aug 2025 - Nov 2025',
    location: 'Stockholm, Stockholm County, Sweden · Remote',
    description: 'Built and delivered a full-stack GEO auditing application that evaluated product-page URLs and suggested improvements. Led architecture, implementation and integration using React and Node.js, including Chromium-based page extraction. Grouped 304 candidate evaluation parameters by functional behaviour and used LLM-based experiments to investigate their importance and inform scoring.',
  },
  {
    title: 'Full Stack Engineer',
    company: 'ChessVerst',
    employment: 'Part-time',
    dates: 'Feb 2021 - Jul 2023',
    location: 'Pune District, Maharashtra, India · Hybrid',
    description: 'Translated product requirements into specifications, wireframes and application architecture for a chess community platform. Developed frontend interfaces, backend services, database functionality and APIs. Worked across UI/UX, testing and integration to bring the platform together.',
  },
  {
    title: 'Associate Software Engineer',
    company: 'Lyte Calc Technologies Pvt Ltd',
    employment: 'Internship',
    dates: 'Mar 2022 - Aug 2022',
    location: 'Bengaluru, Karnataka, India · Remote',
    description: 'Developed e-commerce application functionality using React, Node.js, Express and MongoDB, covering frontend interfaces and backend integration. Completed and handed over a reusable e-commerce application template at the company’s request. Also participated in candidate interviews for the project team.',
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

      {experience.map(role => (
        <div className="space-y-4" key={role.company}>
          <div className="space-y-1.5">
            <div className="flex flex-col sm:flex-row sm:items-baseline justify-between gap-1">
              <h3 className="text-base sm:text-lg font-bold text-[var(--fg-primary)]">{role.title}</h3>
              <span className="text-xs text-[var(--fg-muted)] shrink-0">{role.dates}</span>
            </div>
            <div className="flex flex-wrap items-center gap-x-3 text-xs sm:text-sm text-[var(--accent-cyan)] font-medium">
              <span>{role.company}{role.employment && ` · ${role.employment}`}</span>
              <span className="text-[var(--border-default)]">·</span>
              <span className="text-[var(--fg-muted)]">{role.location}</span>
            </div>
          </div>
          <p className="text-sm sm:text-base leading-relaxed text-[var(--fg-secondary)]">
            {role.description}
          </p>
        </div>
      ))}
    </section>
  );
});

Experience.displayName = 'Experience';
export default Experience;
