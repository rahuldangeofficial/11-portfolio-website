import React, { useState, useEffect, useMemo } from 'react';
import { IconMail, IconPhone, IconGithub, IconLinkedin, IconDownload, IconTerminal } from './Icons';

const Header: React.FC = React.memo(() => {
  
  const [activeSection, setActiveSection] = useState('about');

  const navigationItems = useMemo(() => [
    { id: 'about', label: 'About' },
    { id: 'projects', label: 'Projects' },
    { id: 'experience', label: 'Experience' },
    { id: 'skills', label: 'Skills' },
    { id: 'education', label: 'Education' },
    { id: 'publications', label: 'Publications' },
    { id: 'contact', label: 'Contact' }
  ], []);

  useEffect(() => {
    const sectionIds = navigationItems.map(item => item.id);

    const observerOptions = {
      root: null,
      rootMargin: '-15% 0px -70% 0px',
      threshold: 0
    };

    const visibleSections = new Map<string, number>();
    let footerVisible = false;
    const updateActiveSection = () => {
      if (footerVisible) {
        setActiveSection('contact');
      } else if (visibleSections.size) {
        const first = [...visibleSections].sort((a, b) => a[1] - b[1])[0];
        setActiveSection(first[0]);
      }
    };
    const observerCallback = (entries: IntersectionObserverEntry[]) => {
      for (const entry of entries) {
        if (entry.isIntersecting) {
          visibleSections.set(entry.target.id, sectionIds.indexOf(entry.target.id));
        } else {
          visibleSections.delete(entry.target.id);
        }
      }
      updateActiveSection();
    };

    const observer = new IntersectionObserver(observerCallback, observerOptions);

    sectionIds.forEach((id) => {
      const el = document.getElementById(id);
      if (el) observer.observe(el);
    });

    // Observe the existing footer instead of reading page geometry on every scroll.
    const footerObserver = new IntersectionObserver(([entry]) => {
      footerVisible = entry.isIntersecting;
      updateActiveSection();
    });
    const footer = document.querySelector('#contact footer');
    if (footer) footerObserver.observe(footer);

    return () => {
      observer.disconnect();
      footerObserver.disconnect();
    };
  }, [navigationItems]);

  return (
    <>
      {/* ── HERO BANNER ── */}
      <header className="pt-12 sm:pt-16 pb-8 border-b border-[var(--border-default)]">
        <div className="max-w-4xl mx-auto px-5 sm:px-8 lg:px-8 space-y-6">
          
          {/* Top Status */}
          <div className="flex flex-wrap items-center gap-2 text-xs text-[var(--fg-muted)]">
            <span className="w-2 h-2 rounded-full bg-emerald-500" />
            <span>Sydney, Australia</span>
            <span>·</span>
            <span>Full Work Rights (Subclass 485)</span>
            <span>·</span>
            <span className="text-[var(--accent-cyan)] font-medium">Open to junior / graduate roles</span>
          </div>

          {/* Identity & Headline */}
          <div className="space-y-2">
            <h1 className="text-4xl sm:text-5xl font-extrabold tracking-tight text-[var(--fg-primary)]">
              Rahul Dange
            </h1>
            <p className="text-base sm:text-lg font-medium text-[var(--fg-secondary)]">
              Computer Engineer | Systems Programming · Algorithms · Software Architecture
            </p>
          </div>

          {/* Bio */}
          <div className="space-y-3 text-sm sm:text-base leading-relaxed text-[var(--fg-secondary)] max-w-3xl">
            <p>
              Computer Science graduate from the University of Sydney with a strong foundation in systems programming and low-level software development. Proficient in C, C++, memory management, concurrency, and core computer science fundamentals.
            </p>
            <p>
              Also experienced in full-stack web development using JavaScript, Node.js, Express, MongoDB, and React. Comfortable working across both systems-level and application-level software.
            </p>
          </div>

          {/* Contact */}
          <div className="flex flex-wrap items-center gap-y-2 gap-x-5 text-xs sm:text-sm text-[var(--fg-secondary)]">
            <a href="mailto:contact@rahuldange.com" className="flex items-center gap-1.5 hover:text-[var(--accent-cyan)] transition-colors">
              <IconMail size={14} />
              <span>contact@rahuldange.com</span>
            </a>
            <a href="tel:+61412954045" className="flex items-center gap-1.5 hover:text-[var(--accent-cyan)] transition-colors">
              <IconPhone size={14} />
              <span>+61 412 954 045</span>
            </a>
            <a href="https://github.com/rahuldangeofficial" target="_blank" rel="noopener noreferrer" className="flex items-center gap-1.5 hover:text-[var(--accent-cyan)] transition-colors">
              <IconGithub size={14} />
              <span>GitHub</span>
            </a>
            <a href="https://www.linkedin.com/in/rahuldangeofficial/" target="_blank" rel="noopener noreferrer" className="flex items-center gap-1.5 hover:text-[var(--accent-cyan)] transition-colors">
              <IconLinkedin size={14} />
              <span>LinkedIn</span>
            </a>
          </div>

          {/* Actions */}
          <div className="flex flex-wrap items-center gap-3 pt-1">
            <a
              id="btn-download-resume"
              href="/resume.v4.pdf"
              download="resume.v4.pdf"
              className="btn btn-primary btn-sm flex items-center gap-2 text-xs"
            >
              <IconDownload size={14} />
              <span>Download Resume</span>
            </a>

            <a
              href="#projects"
              className="btn btn-secondary btn-sm text-xs flex items-center gap-1.5"
            >
              <IconTerminal size={14} />
              <span>View Projects</span>
            </a>
          </div>

        </div>
      </header>

      {/* ── STICKY NAVIGATION ── */}
      <nav className="sticky top-0 z-50 bg-white border-b border-[var(--border-default)] py-2.5">
        <div className="max-w-4xl mx-auto px-5 sm:px-8 lg:px-8 flex items-center gap-4">
          <div className="flex items-center gap-2 text-sm font-semibold text-[var(--fg-primary)] shrink-0">
            <span>Rahul Dange</span>
          </div>

          <div className="flex items-center gap-1 text-[11px] sm:text-xs overflow-x-auto hide-scrollbar pb-1 sm:pb-0 w-full sm:w-auto">
            {navigationItems.map((item) => (
              <a
                href={`#${item.id}`}
                aria-current={activeSection === item.id ? 'location' : undefined}
                key={item.id}
                className={`px-2.5 py-1 rounded-full whitespace-nowrap transition-colors ${
                  activeSection === item.id
                    ? 'bg-[var(--fg-primary)] text-[var(--bg-primary)] font-bold'
                    : 'text-[var(--fg-secondary)] hover:text-[var(--fg-primary)] hover:bg-[var(--bg-surface)]'
                }`}
              >
                {item.label}
              </a>
            ))}
          </div>
        </div>
      </nav>
    </>
  );
});

Header.displayName = 'Header';
export default Header;