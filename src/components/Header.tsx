import React, { useState, useEffect, useMemo } from 'react';
import { IconMail, IconGithub, IconLinkedin, IconDownload, IconTerminal } from './Icons';

const Header: React.FC = React.memo(() => {
  
  const [isDark, setIsDark] = useState(false);
  const [themeMode, setThemeMode] = useState('light');
  useEffect(() => {
    try {
      const saved = localStorage.getItem('portfolio-theme-mode');
      if (saved === 'light' || saved === 'dark') setThemeMode(saved);
    } catch { /* Light is the default when storage is unavailable. */ }
  }, []);
  useEffect(() => {
    const sync = () => {
      document.documentElement.dataset.theme = themeMode;
      setIsDark(themeMode === 'dark');
    };
    const delay = window.matchMedia('(prefers-reduced-motion: reduce)').matches ? 0 : 80;
    const timer = window.setTimeout(sync, delay);
    return () => window.clearTimeout(timer);
  }, [themeMode]);
  const chooseTheme = (mode: string) => {
    setThemeMode(mode);
    try { localStorage.setItem('portfolio-theme-mode', mode); } catch { /* Selection still works. */ }
  };

  const [activeSection, setActiveSection] = useState('about');

  const navigationItems = useMemo(() => [
    { id: 'about', label: 'About' },
    { id: 'projects', label: 'Projects' },
    { id: 'experience', label: 'Experience' },
    { id: 'skills', label: 'Skills' },
    { id: 'education', label: 'Education' },
    { id: 'publications', label: 'Publications' },
    { id: 'volunteering', label: 'Volunteering' },
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
      <header className="relative pt-12 sm:pt-16 pb-8 border-b border-[var(--border-default)]">
        <div className="max-w-4xl mx-auto px-5 sm:px-8 lg:px-8 space-y-6">
          
          {/* Identity & Headline */}
          <div className="space-y-2">
            <div className="hero-title-row">
            <h1 className="text-4xl sm:text-5xl font-extrabold tracking-tight text-[var(--fg-primary)]">
              Rahul Dange
            </h1>
        <div className="hero-theme-position">
          <button type="button" className="appearance-toggle" role="switch"
            aria-label="Dark theme" title="Switch light or dark appearance" aria-checked={isDark}
            onClick={() => chooseTheme(document.documentElement.dataset.theme === 'dark' ? 'light' : 'dark')}>
            <span className="appearance-selection" aria-hidden="true" />
            <span className="appearance-option">Light</span>
            <span className="appearance-option">Dark</span>
          </button>
        </div>
            </div>
            <p className="text-base sm:text-lg font-medium text-[var(--fg-secondary)]">
              <span className="hero-disciplines">
                <span className="discipline-row"><span>Computer Engineer</span><span aria-hidden="true">|</span><span>Systems Programming</span></span>
                <span aria-hidden="true" className="discipline-row-separator">·</span>
                <span className="discipline-row"><span>Algorithms</span><span aria-hidden="true">·</span><span>Software Architecture</span></span>
              </span>
            </p>
          </div>

          {/* Bio */}
          <div className="hero-copy space-y-3 text-sm sm:text-base leading-relaxed text-[var(--fg-secondary)] max-w-none">
            <p>
              I’m a computer engineer, a systems experimenter and an Academic Tutor at the University of Sydney.
            </p>
            <p>
              I spend most of my time building software, learning how it works and helping others understand it. My focus is C/C++, memory management, concurrency, filesystems and language implementation. I explore ideas by building, testing and comparing different designs.
            </p>
            <p>
              I also build web applications with React, Node.js and databases. I’ve developed e-commerce software, built platforms for students and led an industry capstone from requirements to a working application.
            </p>
            <p>
              Building and taking initiative have gone together throughout my studies. I started a student resource-sharing app and a cybersecurity club, coordinated a college hackathon, and led several project teams. Today, I also teach C and systems programming.
            </p>
            <p>
              I hold a master’s in computer science from the University of Sydney. I’m looking for junior or graduate software engineering roles where I can contribute to systems, backend services or full-stack applications.
            </p>
            <p>
              <span className="hero-location"><span>Sydney, Australia · Open to relocation</span><span className="location-separator" aria-hidden="true"> · </span><span>Full work rights (Australia)</span></span>
            </p>
          </div>

          {/* Contact */}
          <div className="flex flex-wrap items-center gap-y-2 gap-x-5 text-xs sm:text-sm text-[var(--fg-secondary)]">
            <a href="mailto:contact@rahuldange.com" className="flex items-center gap-1.5 hover:text-[var(--accent-cyan)] transition-colors">
              <IconMail size={14} />
              <span>contact@rahuldange.com</span>
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
          <div className="hero-actions flex flex-wrap items-center gap-3 pt-1">
            <a
              id="btn-download-resume"
              href="/resume.pdf"
              download="resume.pdf"
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
      <nav className="sticky top-0 z-50 bg-[var(--bg-glass-nav)] border-b border-[var(--border-default)] py-2.5">
        <div className="portfolio-nav max-w-4xl mx-auto px-5 sm:px-8 lg:px-8 flex items-center gap-4">

          <div className="nav-section-links flex items-center gap-1 text-xs min-w-0 flex-1">
            {navigationItems.map((item) => (
              <a
                href={`#${item.id}`}
                aria-current={activeSection === item.id ? 'location' : undefined}
                key={item.id}
                className={`px-2.5 py-1 rounded-md whitespace-nowrap transition-colors ${
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