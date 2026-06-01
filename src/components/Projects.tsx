import React from 'react';
import { IconGithub, IconCheckCircle } from './Icons';

interface Project {
  title: string;
  category: 'System Programming' | 'Web Development' | 'Academic Research';
  status: 'Production' | 'Open Source' | 'Research';
  description: string;
  highlights: string[];
  technologies: string[];
  github: string;
  demo: string;
  stars: number;
  featured: boolean;
  academic?: boolean;
}

const projects: Project[] = [
  // Featured Projects (4)
  {
    title: 'Spotlight',
    category: 'Web Development',
    status: 'Open Source',
    description: 'An open-source interaction platform designed for small-scale communities. Features full-stack real-time messaging, customizable topic channels, a personalized news feed, JWT-based role authentication, and custom administrative branding configurations.',
    highlights: [
      'Socket.io real-time direct messaging and interactive browser notifications',
      'Role-based access control with secure JWT authentication layers',
      'Customizable user profiles, avatars, and cover photos',
      'Administrative control panel for community white-label customization'
    ],
    technologies: ['Next.js', 'Express', 'MongoDB', 'Socket.io', 'Redux', 'Passport.js', 'TailwindCSS'],
    github: 'https://github.com/rahuldangeofficial/12-spotlight-interaction-platform',
    demo: 'https://github.com/rahuldangeofficial/12-spotlight-interaction-platform',
    stars: 142,
    featured: true
  },
  {
    title: 'GSDB',
    category: 'System Programming',
    status: 'Open Source',
    description: 'High-performance embedded document database driven by a constant-time geometric engine. Maps 64-bit FNV-1a keys into 4D coordinates on a Space-Filling Curve (SFC) to compute direct disk offsets, achieving true O(1) lookups.',
    highlights: [
      'O(1) record lookups and ingestion, independent of dataset sizing',
      'Ingestion speeds exceeding 1.4 Million operations per second on Apple Silicon',
      'Average lookup latency of 1 microsecond for systems programming tasks',
      'Bypasses logarithmic B-Tree indexing overhead for predictable latency'
    ],
    technologies: ['C++', 'Rust', 'Geometric Indexing', 'Space-Filling Curves', 'Systems Programming'],
    github: 'https://github.com/rahuldangeofficial/15-GSDB-CLI-experimental',
    demo: 'https://github.com/rahuldangeofficial/15-GSDB-CLI-experimental',
    stars: 268,
    featured: true
  },
  {
    title: 'Sentinel Daemon',
    category: 'System Programming',
    status: 'Open Source',
    description: 'Modular systems security daemon and control utility written in C and Assembly. Mimics a biological immune system, running a persistent 2-second patrol loop executing code write-protection, priority throttling, and memory optimization.',
    highlights: [
      'Executable text segment write-protection preventing runtime code injection',
      'CPU scheduler priority nice-tuning (fever response) throttling runaway background indexers',
      'Memory guard page integrity checks using ARM64 and x86_64 inline assembly',
      'Active dictionary-based process security sweeps and automatic page cache purges'
    ],
    technologies: ['C', 'Assembly', 'POSIX Threads', 'Unix Sockets', 'System Optimization', 'Security Daemons'],
    github: 'https://github.com/rahuldangeofficial/18-sentinel-daemon-experimental',
    demo: 'https://github.com/rahuldangeofficial/18-sentinel-daemon-experimental',
    stars: 395,
    featured: true
  },
  {
    title: 'SpatialPress',
    category: 'System Programming',
    status: 'Open Source',
    description: 'High-performance lossless binary compression engine. Chunking binary streams into 8-byte blocks modeled as coordinates in a 4-dimensional discrete vector space, it uses Morton Z-Order Curves and adaptive range coding.',
    highlights: [
      'Bijective Morton coordinate interleaving to preserve multi-dimensional spatial locality',
      'Modular delta encoding to minimize sequence variance prior to encoding',
      'Adaptive 64-bit arithmetic range coder achieving near Shannon entropy limits',
      'Memory-safe stream parsing preventing truncation and double-compression overhead'
    ],
    technologies: ['C++', 'Morton Z-Order', 'Adaptive Range Coding', 'Binary Serialization', 'Algorithms'],
    github: 'https://github.com/rahuldangeofficial/20-spatialpress-CE-experimental',
    demo: 'https://github.com/rahuldangeofficial/20-spatialpress-CE-experimental',
    stars: 184,
    featured: true
  },

  // Additional Projects (4)
  {
    title: 'edit (Terminal Text Editor)',
    category: 'System Programming',
    status: 'Open Source',
    description: 'A minimalist, zero-config terminal text editor native to UTF-8 and atomically safe. Built in C to achieve a tiny 64 KB memory binary footprint (84x smaller than Vim), it supports crash recovery buffer saves and mouse integration. (Personal Proud Project).',
    highlights: [
      'Atomically safe file saves (write ➔ fsync ➔ rename) preventing buffer loss or corruption',
      'Bypasses memory bloat, operating on a ~1-2 MB RAM footprint at runtime',
      'Crash-safe mechanisms intercepting signals (like Ctrl+C) to automatically save changes before exit',
      'UTF-8 and Emoji native rendering with dynamic-width line numbers and mouse support'
    ],
    technologies: ['C', 'POSIX APIs', 'Terminal I/O', 'Buffer Management', 'UTF-8', 'Systems Programming'],
    github: 'https://github.com/rahuldangeofficial/14-edit-text-editor',
    demo: 'https://github.com/rahuldangeofficial/14-edit-text-editor',
    stars: 432,
    featured: false
  },
  {
    title: 'Word Arithmetic Parser',
    category: 'Web Development',
    status: 'Production',
    description: 'An educational compiler visualizer and single-page dashboard parsing and evaluating arithmetic expressions written in natural English words (e.g. "ten plus two multiply three"). Enforces BODMAS operator precedence using a custom recursive descent parser.',
    highlights: [
      'Custom hand-written ES Module lexical tokenizer and non-backtracking recursive descent parser',
      'Interactive Abstract Syntax Tree (AST) node graphs rendered dynamically via SVG connectors',
      'BODMAS evaluation playback player to step through tree reduction step-by-step',
      'Case-insensitive, whitespace-agnostic parsing supporting float/decimal numbers'
    ],
    technologies: ['React', 'JavaScript', 'HTML5/SVG', 'Compiler Design', 'AST Visualizers'],
    github: 'https://github.com/rahuldangeofficial/09-word-arithmetic-parser',
    demo: 'https://09-word-arithmetic-parser.vercel.app/',
    stars: 114,
    featured: false
  },
  {
    title: 'EasyKeeper',
    category: 'Web Development',
    status: 'Production',
    description: 'A minimalist note-taking application featuring passwordless Google Sign-In and real-time cloud data storage synchronization. Fully optimized for instant loading, offline access, and responsive layout flows.',
    highlights: [
      'Seamless authentication integrations using Google Identity services',
      'Real-time database sync via Firebase Firestore with offline cache persistence',
      'Clean, distraction-free modern note editor with auto-saving capability',
      'Highly optimized Next.js client-side page transitions and session loads'
    ],
    technologies: ['React', 'Next.js', 'Firebase', 'Vercel', 'TailwindCSS', 'Authentication'],
    github: 'https://github.com/rahuldangeofficial/07-easykeeper',
    demo: 'https://easykeeper.vercel.app/',
    stars: 92,
    featured: false
  },
  {
    title: 'LLFPL1 Compiler (WIP)',
    category: 'System Programming',
    status: 'Research',
    description: 'An experimental systems compiler for a Low-Level Functional Programming Language. Bypasses standard garbage collection, heap fragmentation, and runtime overhead by treating memory as CPU cache-aligned geometry. (Currently in active development).',
    highlights: [
      'Rejects dynamic heap allocators in favor of static, cache-aligned L1/L2 memory slabs',
      'Zero-copy pointer-offset lexical scanner processing 100MB of source code per second',
      'Predicated execution logic evaluating branches via bitwise masks without CPU pipeline stalls',
      'Tail-call optimization replacing call routines with static register jump operations'
    ],
    technologies: ['C', 'Assembly', 'CPU Caching', 'AVX-512', 'Compiler Design', 'Systems Optimization'],
    github: 'https://github.com/rahuldangeofficial/16-LLFPL1-experimental',
    demo: 'https://github.com/rahuldangeofficial/16-LLFPL1-experimental',
    stars: 78,
    featured: false
  }
];

const getCategoryTagClass = (category: string): string => {
  switch (category) {
    case 'System Programming': return 'tag tag-blue';
    case 'Web Development': return 'tag tag-green';
    case 'Academic Research': return 'tag';
    default: return 'tag';
  }
};

const getStatusTagClass = (status: string): string => {
  switch (status) {
    case 'Production': return 'tag tag-green';
    case 'Open Source': return 'tag tag-blue';
    case 'Research': return 'tag';
    default: return 'tag';
  }
};

const getAccentClasses = (category: string): string => {
  return 'glow-blue';
};

const featuredProjects = projects.filter(p => p.featured);
const regularProjects = projects.filter(p => !p.featured && !p.academic);
const academicProjects = projects.filter(p => p.academic);

const FeaturedCard: React.FC<{ project: Project }> = React.memo(({ project }) => (
  <article className={`card-elevated scroll-reveal ${getAccentClasses(project.category)} p-4 sm:p-6 md:p-8 flex flex-col`}>
    <div className="flex items-start justify-between mb-4">
      <div className="flex-1">
        <div className="flex items-center gap-2 mb-2 flex-wrap">
          <span className={getCategoryTagClass(project.category)}>{project.category}</span>
          <span className={getStatusTagClass(project.status)}>{project.status}</span>
        </div>
        <h4 className="text-xl md:text-2xl font-bold text-[var(--fg-primary)]">
          {project.title}
        </h4>
      </div>
    </div>

    <p className="mb-4 leading-relaxed text-sm md:text-base text-[var(--fg-secondary)]">
      {project.description}
    </p>

    <div className="mb-4">
      <h5 className="text-base font-semibold mb-3 text-[var(--fg-primary)]">Key Highlights</h5>
      <ul className="space-y-2">
        {project.highlights.map((highlight, i) => (
          <li key={i} className="flex items-start gap-3">
            <IconCheckCircle size={14} className="flex-shrink-0 mt-0.5 text-[var(--accent-green)]" />
            <span className="text-sm text-[var(--fg-secondary)]">{highlight}</span>
          </li>
        ))}
      </ul>
    </div>

    <div className="mb-4">
      <h5 className="text-base font-semibold mb-3 text-[var(--fg-primary)]">Technologies</h5>
      <div className="flex flex-wrap gap-1.5">
        {project.technologies.map((tech, i) => (
          <span key={i} className="skill-tag">{tech}</span>
        ))}
      </div>
    </div>

    <div className="flex gap-3 pt-4 mt-auto border-t border-[var(--border-subtle)]">
      <a
        id={`project-github-${project.title.toLowerCase().replace(/\s+/g, '-')}`}
        href={project.github}
        target="_blank"
        rel="noopener noreferrer"
        className="btn btn-primary btn-sm flex items-center gap-2"
      >
        <IconGithub size={14} />
        <span>View Code</span>
      </a>
    </div>
  </article>
));

const RegularCard: React.FC<{ project: Project }> = React.memo(({ project }) => (
  <article className="card scroll-reveal p-4 sm:p-6 flex flex-col h-full">
    <div className="flex items-start justify-between mb-3">
      <div className="flex-1">
        <div className="flex items-center gap-2 mb-2 flex-wrap">
          <span className={getCategoryTagClass(project.category)}>{project.category}</span>
          <span className={getStatusTagClass(project.status)}>{project.status}</span>
        </div>
        <h4 className="text-lg font-bold text-[var(--fg-primary)]">
          {project.title}
        </h4>
      </div>
    </div>

    <p className="mb-4 leading-relaxed text-sm flex-1 text-[var(--fg-secondary)]">
      {project.description}
    </p>

    <div className="mb-4">
      <div className="flex flex-wrap gap-1.5">
        {project.technologies.slice(0, 4).map((tech, i) => (
          <span key={i} className="skill-tag">{tech}</span>
        ))}
        {project.technologies.length > 4 && (
          <span className="skill-tag">+{project.technologies.length - 4} more</span>
        )}
      </div>
    </div>

    <div className="flex gap-2 pt-3 mt-auto border-t border-[var(--border-subtle)]">
      <a
        id={`project-github-${project.title.toLowerCase().replace(/\s+/g, '-')}`}
        href={project.github}
        target="_blank"
        rel="noopener noreferrer"
        className="btn btn-primary btn-sm flex items-center gap-2"
      >
        <IconGithub size={14} />
        <span>View Code</span>
      </a>
    </div>
  </article>
));

const AcademicCard: React.FC<{ project: Project }> = React.memo(({ project }) => (
  <article className="card scroll-reveal accent-left-blue glow-blue p-4 sm:p-6 flex flex-col h-full">
    <div className="flex items-start justify-between mb-3">
      <div className="flex-1">
        <div className="flex items-center gap-2 mb-2 flex-wrap">
          <span className={getCategoryTagClass(project.category)}>{project.category}</span>
          <span className={getStatusTagClass(project.status)}>{project.status}</span>
        </div>
        <h4 className="text-lg font-bold text-[var(--fg-primary)]">
          {project.title}
        </h4>
      </div>
    </div>

    <p className="text-sm mb-4 flex-1 text-[var(--fg-secondary)]">
      {project.description}
    </p>

    <div className="mb-4">
      <h5 className="text-sm font-semibold mb-2 text-[var(--fg-primary)]">Key Achievements:</h5>
      <ul className="text-xs space-y-1 text-[var(--fg-secondary)]">
        {project.highlights.slice(0, 3).map((highlight, i) => (
          <li key={i} className="flex items-start">
            <span className="mr-2 text-[var(--accent-blue)]">•</span>
            {highlight}
          </li>
        ))}
      </ul>
    </div>

    <div className="mb-4">
      <div className="flex flex-wrap gap-1.5">
        {project.technologies.slice(0, 6).map((tech, i) => (
          <span key={i} className="skill-tag">{tech}</span>
        ))}
      </div>
    </div>

    <div className="flex gap-2 pt-3 mt-auto border-t border-[var(--border-subtle)]">
      <a
        id={`project-github-${project.title.toLowerCase().replace(/\s+/g, '-')}`}
        href={project.github}
        target="_blank"
        rel="noopener noreferrer"
        className="btn btn-primary btn-sm flex items-center gap-1.5"
      >
        <IconGithub size={14} />
        <span>View Code</span>
      </a>
    </div>
  </article>
));

const Projects: React.FC = React.memo(() => {
  return (
    <section id="projects" className="section section-alt content-auto">
      <div className="container">
        <div className="max-w-4xl mx-auto">
          <h2 className="section-heading">Featured Projects</h2>
          <p className="section-subheading">
            A selection of systems programming tools, web platforms, and research projects I've designed and built.
          </p>

          {/* Featured Projects */}
          <div className="mb-8 md:mb-12">
            <h3 className="text-xl font-semibold mb-6 pb-2 border-b border-[var(--border-subtle)] text-[var(--fg-primary)]">
              Highlighted Work
            </h3>
            <div className="grid grid-cols-1 lg:grid-cols-2 gap-6 md:gap-8">
              {featuredProjects.map((project, index) => (
                <FeaturedCard key={index} project={project} />
              ))}
            </div>
          </div>

          {/* Regular Projects */}
          <div className="mb-8 md:mb-12">
            <h3 className="text-xl font-semibold mb-6 pb-2 border-b border-[var(--border-subtle)] text-[var(--fg-primary)]">
              Additional Projects
            </h3>
            <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
              {regularProjects.map((project, index) => (
                <RegularCard key={index} project={project} />
              ))}
            </div>
          </div>

          {/* Academic/Research Projects */}
          {academicProjects.length > 0 && (
            <div>
              <h3 className="text-xl font-semibold mb-6 pb-2 border-b border-[var(--border-subtle)] text-[var(--fg-primary)]">
                Academic &amp; Research Projects
              </h3>
              <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
                {academicProjects.map((project, index) => (
                  <AcademicCard key={index} project={project} />
                ))}
              </div>
            </div>
          )}
        </div>
      </div>
    </section>
  );
});

export default Projects;
