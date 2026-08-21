import React, { useState } from 'react';
import { IconGithub, IconExternalLink, IconTerminal } from './Icons';
import LlfplPlayground from './playgrounds/LlfplPlayground';
import MimfsPlayground from './playgrounds/MimfsPlayground';

interface SecondaryProjectItem {
  title: string;
  tags: string;
  description: string;
  bullets: string[];
  github: string;
  demo?: string;
}

const secondaryProjects: SecondaryProjectItem[] = [
  {
    title: 'GSDB: Constant-Time Geometric Document Database',
    tags: 'C, Space-Filling Curves',
    description: 'Embedded document database driven by a constant-time geometric indexing engine. Maps 64-bit FNV-1a keys onto a 4D Space-Filling Curve (SFC) to calculate direct disk offsets in O(1) time.',
    bullets: [
      'O(1) lookups and ingestion, independent of dataset volume.',
      'Sustained ingestion throughput exceeding 1.4 Million operations/sec on Apple Silicon.',
      '1 microsecond average lookup latency, bypassing B-Tree index traversal.'
    ],
    github: 'https://github.com/rahuldangeofficial/15-GSDB-CLI-experimental'
  },
  {
    title: 'Sentinel Daemon: Systems Security Utility',
    tags: 'C, Assembly, POSIX',
    description: 'Low-overhead security daemon executing automated runtime memory integrity patrol and executable text segment write-protection.',
    bullets: [
      'Read-only enforcement on executable code segments to prevent runtime code modification.',
      'Process scheduler tuning to throttle runaway background threads.',
      'Guard page validation routines using ARM64 and x86-64 inline assembly.'
    ],
    github: 'https://github.com/rahuldangeofficial/18-sentinel-daemon-experimental'
  },
  {
    title: 'edit: Minimalist Terminal Text Editor',
    tags: 'C, POSIX, UTF-8',
    description: 'Minimalist, zero-config terminal text editor native to UTF-8 with crash-safe persistence. Compiles into a 64 KB memory binary footprint.',
    bullets: [
      'Atomic save cycle (write -> fsync -> rename) preventing buffer loss or disk corruption.',
      'Operates within a 1-2 MB RAM runtime memory envelope.',
      'Signal handlers intercepting abnormal terminations to save dirty buffers before exit.'
    ],
    github: 'https://github.com/rahuldangeofficial/14-edit-text-editor'
  },
  {
    title: 'Spotlight: Real-Time Interaction Platform',
    tags: 'Next.js, Express, MongoDB, Socket.io',
    description: 'Full-stack community communication platform with real-time bidirectional messaging, customizable topic channels, and role-based access control.',
    bullets: [
      'Socket.io channel multiplexing with sub-50ms message broadcast latency.',
      'Role-based authorization and session verification using JSON Web Tokens (JWT).',
      'Administrative dashboard for permission management and platform configuration.'
    ],
    github: 'https://github.com/rahuldangeofficial/12-spotlight-interaction-platform',
    demo: 'https://github.com/rahuldangeofficial/12-spotlight-interaction-platform'
  },
  {
    title: 'Word Arithmetic Parser',
    tags: 'React, TypeScript, Compiler Design',
    description: 'Educational compiler visualizer parsing and evaluating arithmetic expressions written in natural English words, enforcing standard BODMAS operator precedence.',
    bullets: [
      'Lexical tokenizer with non-backtracking recursive descent grammar parser.',
      'Dynamic Abstract Syntax Tree (AST) visualization rendered via SVG connectors.',
      'Step-by-step reduction stepper showing tree simplifications in real time.'
    ],
    github: 'https://github.com/rahuldangeofficial/09-word-arithmetic-parser',
    demo: 'https://09-word-arithmetic-parser.vercel.app/'
  },
  {
    title: 'EasyKeeper: Minimalist Cloud Notes',
    tags: 'React, Next.js, Firebase',
    description: 'Minimalist note-taking application featuring Google Identity authentication and real-time offline-first cloud database synchronization.',
    bullets: [
      'Passwordless authentication using Google Identity Services.',
      'Real-time document synchronization via Firebase Firestore with local cache fallback.',
      'Clean, distraction-free text editor with automatic draft saving.'
    ],
    github: 'https://github.com/rahuldangeofficial/07-easykeeper',
    demo: 'https://easykeeper.vercel.app/'
  }
];

const Projects: React.FC = React.memo(() => {
  const [playgrounds, setPlaygrounds] = useState<{mimfs: boolean, llfpl: boolean}>({
    mimfs: true,
    llfpl: true
  });

  const togglePlayground = (key: 'mimfs' | 'llfpl') => {
    setPlaygrounds(prev => ({ ...prev, [key]: !prev[key] }));
  };

  return (
    <section id="projects" className="space-y-12">
      <div className="pb-2 border-b border-[var(--border-default)]">
        <h2 className="text-xl sm:text-2xl font-bold tracking-tight text-[var(--fg-primary)]">
          Projects
        </h2>
      </div>

      {/* ═══════════════════════════════════════════════════════════════
          PRIMARY FLAGSHIP PROJECTS (MIMFS & LLFPL)
          ═══════════════════════════════════════════════════════════════ */}
      <div className="space-y-10">
        
        {/* ── MIMFS ── */}
        <div className="space-y-3 pb-8 border-b border-[var(--border-subtle)]">
          <div className="flex flex-col sm:flex-row sm:items-baseline justify-between gap-1">
            <div className="space-y-0.5">
              <h3 className="text-base sm:text-lg font-bold text-[var(--fg-primary)] flex items-center gap-2 flex-wrap">
                <a
                  href="https://github.com/rahuldangeofficial/MIMFS"
                  target="_blank"
                  rel="noopener noreferrer"
                  className="hover:underline text-[var(--fg-primary)] hover:text-[var(--accent-cyan)]"
                >
                  MIMFS (Multithreaded In Memory File System)
                </a>
              </h3>
              <div className="text-xs  text-[var(--accent-cyan)] font-medium">
                C11, POSIX Threads
              </div>
            </div>

            <div className="flex items-center gap-2 pt-1 sm:pt-0">
              <button
                onClick={() => togglePlayground('mimfs')}
                className="btn btn-primary btn-sm text-xs  flex items-center gap-1.5"
              >
                <IconTerminal size={13} />
                <span>{playgrounds.mimfs ? 'Hide Interactive Shell' : 'Interactive Shell'}</span>
              </button>
              <a
                href="https://github.com/rahuldangeofficial/MIMFS"
                target="_blank"
                rel="noopener noreferrer"
                className="btn btn-secondary btn-sm text-xs  flex items-center gap-1"
              >
                <IconGithub size={13} />
                <span>GitHub</span>
              </a>
            </div>
          </div>

          <ul className="space-y-2 list-disc list-outside pl-4 text-sm sm:text-base leading-relaxed text-[var(--fg-secondary)]">
            <li>
              Thread-safe POSIX file system that operates entirely inside a caller-provided memory buffer with zero dynamic memory allocation (<code className="px-1 py-0.5 rounded bg-[var(--bg-surface)] border border-[var(--border-subtle)] font-mono text-xs">malloc</code>/<code className="px-1 py-0.5 rounded bg-[var(--bg-surface)] border border-[var(--border-subtle)] font-mono text-xs">free</code>). Supports directories, hard and symbolic links, sparse files, and atomic rename.
            </li>
            <li>
              Implements lock-free block and inode allocation using C11 atomics, combined with per-inode mutexes to minimize lock contention across concurrent worker threads.
            </li>
            <li>
              Exhaustively tested with unit and multithreaded stress suites under AddressSanitizer, ThreadSanitizer, and UndefinedBehaviorSanitizer.
            </li>
          </ul>

          {playgrounds.mimfs && (
            <div className="pt-3 animate-fadeIn">
              <MimfsPlayground />
            </div>
          )}
        </div>

        {/* ── LLFPL ── */}
        <div className="space-y-3 pb-8 border-b border-[var(--border-subtle)]">
          <div className="flex flex-col sm:flex-row sm:items-baseline justify-between gap-1">
            <div className="space-y-0.5">
              <h3 className="text-base sm:text-lg font-bold text-[var(--fg-primary)] flex items-center gap-2 flex-wrap">
                <a
                  href="https://github.com/rahuldangeofficial/LLFPL"
                  target="_blank"
                  rel="noopener noreferrer"
                  className="hover:underline text-[var(--fg-primary)] hover:text-[var(--accent-cyan)]"
                >
                  LLFPL (Low-Level Floating-Point Language)
                </a>
              </h3>
              <div className="text-xs  text-[var(--accent-cyan)] font-medium">
                C11, Deterministic Runtime
              </div>
            </div>

            <div className="flex items-center gap-2 pt-1 sm:pt-0">
              <button
                onClick={() => togglePlayground('llfpl')}
                className="btn btn-primary btn-sm text-xs  flex items-center gap-1.5"
              >
                <IconTerminal size={13} />
                <span>{playgrounds.llfpl ? 'Hide Studio REPL' : 'Studio REPL'}</span>
              </button>
              <a
                href="https://github.com/rahuldangeofficial/LLFPL"
                target="_blank"
                rel="noopener noreferrer"
                className="btn btn-secondary btn-sm text-xs  flex items-center gap-1"
              >
                <IconGithub size={13} />
                <span>GitHub</span>
              </a>
            </div>
          </div>

          <ul className="space-y-2 list-disc list-outside pl-4 text-sm sm:text-base leading-relaxed text-[var(--fg-secondary)]">
            <li>
              Interpreter for a deterministic expression language designed for numerical computing. Memory-maps source files and evaluates expressions directly without constructing a syntax tree, minimizing memory overhead.
            </li>
            <li>
              Pre-allocates all memory buffers at startup, guaranteeing zero runtime heap allocations and bit-exact IEEE-754 double precision reproducibility across runs.
            </li>
            <li>
              Verified against expected-output test suites, syntax edge cases, and sanitizer checks.
            </li>
          </ul>

          {playgrounds.llfpl && (
            <div className="pt-3 animate-fadeIn">
              <LlfplPlayground />
            </div>
          )}
        </div>

      </div>

      {/* ═══════════════════════════════════════════════════════════════
          SECONDARY / SPECIALIZED PROJECTS
          ═══════════════════════════════════════════════════════════════ */}
      <div className="space-y-6 pt-2">
        <h3 className="text-base font-bold text-[var(--fg-primary)] pb-1 border-b border-[var(--border-subtle)]">
          Additional Projects &amp; Research
        </h3>

        <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
          {secondaryProjects.map((p, idx) => (
            <div
              key={idx}
              className="p-5 rounded-lg border border-[var(--border-default)] bg-[var(--bg-primary)] flex flex-col justify-between space-y-3 hover:border-[var(--accent-cyan)] transition-colors"
            >
              <div className="space-y-2">
                <div className="flex items-baseline justify-between gap-2">
                  <h4 className="text-base font-bold text-[var(--fg-primary)]">
                    {p.title.split(': ')[0]}
                  </h4>
                  <span className="text-[11px]  text-[var(--fg-muted)]">
                    {p.tags.split(',')[0]}
                  </span>
                </div>

                <div className="text-xs  text-[var(--accent-cyan)]">
                  {p.tags}
                </div>

                <p className="text-sm sm:text-base text-[var(--fg-secondary)] leading-relaxed">
                  {p.description}
                </p>

                <ul className="space-y-1 list-disc list-outside pl-4 text-xs text-[var(--fg-secondary)]">
                  {p.bullets.map((b, bIdx) => (
                    <li key={bIdx}>{b}</li>
                  ))}
                </ul>
              </div>

              <div className="flex items-center gap-2 pt-2 border-t border-[var(--border-subtle)]">
                <a
                  href={p.github}
                  target="_blank"
                  rel="noopener noreferrer"
                  className="btn btn-secondary btn-sm text-xs  flex items-center gap-1.5"
                >
                  <IconGithub size={12} />
                  <span>Code</span>
                </a>
                {p.demo && p.demo !== p.github && (
                  <a
                    href={p.demo}
                    target="_blank"
                    rel="noopener noreferrer"
                    className="btn btn-primary btn-sm text-xs  flex items-center gap-1.5"
                  >
                    <IconExternalLink size={12} />
                    <span>Live</span>
                  </a>
                )}
              </div>
            </div>
          ))}
        </div>
      </div>
    </section>
  );
});

Projects.displayName = 'Projects';
export default Projects;
