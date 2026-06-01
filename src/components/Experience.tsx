import React from 'react';
import { IconBriefcase, IconCalendar, IconMapPin, IconArrowRight } from './Icons';

interface ExperienceItem {
  role: string;
  company: string;
  period: string;
  location: string;
  achievements: string[];
  skills: string[];
}

const experiences: ExperienceItem[] = [
  {
    role: 'Academic Tutor | System Programming (COMP2017)',
    company: 'University of Sydney',
    period: 'Feb 2026 - Present',
    location: 'Camperdown, NSW, Australia (On-site)',
    achievements: [
      'Taught systems programming principles to undergraduate students, focusing on memory layouts, pointers, structures, and low-level mechanics in C.',
      'Guided students in writing concurrent systems using POSIX threads, synchronization primitives (mutexes, semaphores, condition variables), IPC, and signal handling.',
      'Mentored students in using debugging and diagnostic tools like GDB and Valgrind to identify memory leaks, race conditions, and segmentation faults.',
      'Conducted weekly lab sessions, evaluated complex code submissions, and provided detailed constructive feedback on performance optimization.'
    ],
    skills: ['C (Programming Language)', 'Systems Programming', 'Concurrency', 'IPC', 'Linux', 'GDB', 'Valgrind']
  },
  {
    role: 'Team Lead & Lead Systems Architect (USYD Capstone Project)',
    company: 'Aeoflo Company (Industry Partner)',
    period: 'Jul 2025 - Nov 2025',
    location: 'Sydney, NSW, Australia (On-site)',
    achievements: [
      'Led a 7-person engineering team, managing client communications, stakeholder alignment, and translation of product needs from the Product Manager into technical requirements.',
      'Acted as the sole Systems Architect, designing the core platform architecture and building the initial prototype from scratch.',
      'Engineered an innovative web scraping pipeline that loaded dynamic target pages inside custom headless browsers to extract dynamic files and runtime source code.',
      'Key researcher in identifying 304 distinct ranking factors affecting Generative AI search optimization (GEO) across geographical markets.',
      'Served as the weekly team spokesperson, presenting milestones and updates to Aeoflo executives, securing a final grade of Distinction.'
    ],
    skills: ['System Architecture', 'Headless Browsers', 'Web Scraping', 'Generative AI', 'SEO/GEO', 'Team Leadership']
  },
  {
    role: 'Lead Developer / Full Stack Engineer',
    company: 'ChessVerst',
    period: 'Feb 2021 - Jul 2023',
    location: 'Pune, Maharashtra, India (On-site)',
    achievements: [
      'Led the end-to-end platform design, user interaction flows, UI/UX architecture, testing, and deployment for the ChessVerst startup.',
      'Translated vague, non-technical requirements from business stakeholders into structured, development-ready Software Requirement Specifications (SRS) documents.',
      'Designed and created the startup\'s official brand identity and logo.',
      'Implemented clean, modular, and reusable web features using React, TypeScript, and state management libraries.'
    ],
    skills: ['JavaScript (ES6+)', 'TypeScript', 'React', 'Software Architecture', 'UI/UX Design', 'SRS Documentation', 'Logo Design']
  },
  {
    role: 'Associate Software Engineer',
    company: 'Calc Technologies',
    period: 'Mar 2022 - May 2022',
    location: 'Remote',
    achievements: [
      'Designed and engineered the company\'s first reusable e-commerce infrastructure template, increasing development productivity for subsequent project setups.',
      'Created standardized APIs and reusable components, reducing deployment timelines and enhancing overall code reuse.',
      'Recognized and praised directly by Calc Technologies CEO Pawan Kumar for exceptional execution and initiative.'
    ],
    skills: ['JavaScript (ES6+)', 'TypeScript', 'React.js', 'E-Commerce Infrastructure', 'Software Templating']
  }
];

const Experience: React.FC = React.memo(() => {
  return (
    <section id="experience" className="section section-alt content-auto">
      <div className="container">
        <h2 className="section-heading">Work Experience</h2>
        <p className="section-subheading">
          A track record of engineering robust systems, designing platform architectures, and leading development teams.
        </p>

        <div className="max-w-4xl mx-auto space-y-6">
          {experiences.map((exp, idx) => (
            <div key={idx} className="card scroll-reveal p-4 sm:p-6 md:p-8 flex items-start gap-4 sm:gap-6 text-left">
              <div
                className="w-10 h-10 sm:w-12 sm:h-12 rounded-full flex items-center justify-center flex-shrink-0 bg-[var(--accent-cyan-glow-strong)]"
              >
                <span className="text-[var(--accent-cyan)]">
                  <IconBriefcase size={18} className="sm:hidden" />
                  <IconBriefcase size={20} className="hidden sm:block" />
                </span>
              </div>
              <div className="flex-1 space-y-4">
                <div>
                  <h3 className="text-lg sm:text-xl font-bold leading-snug text-[var(--fg-primary)]">
                    {exp.role}
                  </h3>
                  <div className="flex flex-wrap items-center gap-x-3 gap-y-1.5 text-sm mt-1.5">
                    <span className="font-semibold text-[var(--accent-cyan)] w-full sm:w-auto">{exp.company}</span>
                    <span className="hide-on-mobile sm:inline text-[var(--border-default)]">•</span>
                    <span className="flex items-center gap-1.5 text-[var(--fg-muted)] w-full sm:w-auto">
                      <IconCalendar size={14} className="shrink-0" />
                      <span className="whitespace-nowrap">{exp.period}</span>
                    </span>
                    <span className="hide-on-mobile sm:inline text-[var(--border-default)]">•</span>
                    <span className="flex items-center gap-1.5 text-[var(--fg-muted)] w-full sm:w-auto">
                      <IconMapPin size={14} className="shrink-0" />
                      <span className="whitespace-nowrap">{exp.location}</span>
                    </span>
                  </div>
                </div>

                {/* Achievements List */}
                <ul className="space-y-3">
                  {exp.achievements.map((bullet, i) => (
                    <li key={i} className="flex items-start text-sm sm:text-base leading-relaxed text-[var(--fg-secondary)]">
                      <span className="text-[var(--accent-cyan)] mt-1.5 mr-2 flex-shrink-0"><IconArrowRight size={12} /></span>
                      <span>{bullet}</span>
                    </li>
                  ))}
                </ul>

                {/* Tech Skills Badges */}
                <div className="flex flex-wrap gap-1.5 pt-2">
                  {exp.skills.map((skill, i) => (
                    <span key={i} className="skill-tag">
                      {skill}
                    </span>
                  ))}
                </div>
              </div>
            </div>
          ))}
        </div>
      </div>
    </section>
  );
});

Experience.displayName = 'Experience';

export default Experience;
