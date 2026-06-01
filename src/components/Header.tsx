import React from 'react';
import { IconMail, IconPhone, IconMapPin, IconGithub, IconLinkedin, IconDownload, IconMenu, IconX } from './Icons';

const Header: React.FC = React.memo(() => {
  const [isMenuOpen, setIsMenuOpen] = React.useState(false);
  const [activeSection, setActiveSection] = React.useState('about');

  const navigationItems = React.useMemo(() => [
    { id: 'about', label: 'About' },
    { id: 'experience', label: 'Experience' },
    { id: 'skills', label: 'Skills' },
    { id: 'projects', label: 'Projects' },
    { id: 'education', label: 'Education' },
    { id: 'preferences', label: 'Tech Stack' },
    { id: 'interests', label: 'Interests' },
    { id: 'publications', label: 'Publications' },
    { id: 'volunteering', label: 'Volunteering' },
    { id: 'mystory', label: 'My Story' },
    { id: 'contact', label: 'Contact' }
  ], []);

  React.useEffect(() => {
    const sectionIds = navigationItems.map(item => item.id);

    const observerOptions = {
      root: null,
      rootMargin: '-10% 0px -60% 0px',
      threshold: 0
    };

    const observerCallback = (entries: IntersectionObserverEntry[]) => {
      // Find the entry that is currently intersecting the active viewport zone
      const intersecting = entries.filter(entry => entry.isIntersecting);
      if (intersecting.length > 0) {
        // Highlight the top-most intersecting section
        const sorted = intersecting.sort((a, b) => a.boundingClientRect.top - b.boundingClientRect.top);
        setActiveSection(sorted[0].target.id);
      }
    };

    const observer = new IntersectionObserver(observerCallback, observerOptions);

    sectionIds.forEach((id) => {
      const el = document.getElementById(id);
      if (el) observer.observe(el);
    });

    // Highly lightweight scroll listener purely for bottom-page check (no layout thrashing)
    const handleScroll = () => {
      if (window.innerHeight + window.scrollY >= document.documentElement.scrollHeight - 120) {
        setActiveSection(sectionIds[sectionIds.length - 1]);
      }
    };

    window.addEventListener('scroll', handleScroll, { passive: true });

    return () => {
      observer.disconnect();
      window.removeEventListener('scroll', handleScroll);
    };
  }, [navigationItems]);

  const scrollToSection = React.useCallback((sectionId: string) => {
    const element = document.getElementById(sectionId);
    if (element) {
      const globalLenis = (window as any).lenis;
      if (globalLenis) {
        globalLenis.scrollTo(element, {
          offset: -80,
          duration: 1.0,
          easing: (t: number) => Math.min(1, 1.001 - Math.pow(2, -10 * t)) // easeOutExpo
        });
      } else {
        const headerOffset = 80;
        const elementPosition = element.getBoundingClientRect().top + window.scrollY;
        const offsetPosition = elementPosition - headerOffset;
        window.scrollTo({
          top: offsetPosition,
          behavior: 'smooth'
        });
      }
    }
    setIsMenuOpen(false);
  }, []);

  return (
    <>
      {/* Hero Section */}
      <section className="hero py-12 sm:py-16 md:py-24 lg:py-28 relative overflow-hidden">
        <div className="hero-gradient" />
        <div className="hero-grid" />
        <div className="container relative z-10">
          <div className="max-w-4xl mx-auto text-center space-y-6 md:space-y-8 flex flex-col items-center">
            
            {/* Profile Picture */}
            <div className="relative group select-none">
              <div 
                className="absolute inset-0 rounded-full blur-md opacity-20 group-hover:opacity-35 transition-opacity duration-500 bg-black"
              />
              <img 
                src="/profile.png" 
                alt="Rahul Dange" 
                fetchpriority="high"
                className="w-28 h-28 sm:w-32 sm:h-32 md:w-36 md:h-36 rounded-full object-cover border-2 border-[var(--border-default)] group-hover:border-black shadow-lg relative z-10 transition-all duration-500 transform group-hover:scale-[1.02] will-change-transform"
              />
            </div>

            {/* Name */}
            <h1 className="text-5xl sm:text-6xl md:text-7xl font-extrabold tracking-tighter leading-none text-[var(--fg-primary)]">
              Rahul Dange
            </h1>

            {/* Subtitle / Headline (Professional Title) */}
            <p className="text-xl sm:text-2xl md:text-3xl font-normal tracking-tight text-[var(--fg-secondary)] leading-relaxed max-w-3xl">
              Computer Engineer specializing in <span className="font-semibold text-[var(--fg-primary)]">Systems</span>, <span className="font-semibold text-[var(--fg-primary)]">Algorithms</span> &amp; <span className="font-semibold text-[var(--fg-primary)]">Product Architecture</span>
            </p>

            {/* Education Tag (under title) */}
            <div className="inline-flex flex-wrap items-center justify-center gap-x-2 gap-y-1 px-4 py-2 sm:py-1 rounded-2xl sm:rounded-full text-[10px] sm:text-xs font-semibold uppercase tracking-wider bg-[var(--bg-surface)] text-[var(--fg-secondary)] border border-[var(--border-default)] text-center max-w-sm sm:max-w-none">
              <span>University of Sydney</span>
              <span className="hide-on-mobile sm:inline text-[var(--border-default)]">•</span>
              <span>Master of Computer Science</span>
              <span className="w-full sm:w-auto block sm:inline text-[var(--fg-secondary)] opacity-85">
                (Software Engineering Specialization)
              </span>
            </div>

            {/* Contact Info */}
            <div className="flex flex-col sm:flex-row flex-wrap justify-center items-center gap-y-2.5 gap-x-6 text-xs sm:text-sm text-[var(--fg-secondary)] pt-4 border-t border-[var(--border-subtle)] w-full max-w-2xl">
              <a href="mailto:contact@rahuldange.com" className="flex items-center gap-2 hover:text-[var(--accent-cyan)] transition-colors text-[var(--fg-secondary)]">
                <IconMail size={14} />
                <span>contact@rahuldange.com</span>
              </a>
              
              <div className="flex items-center justify-center gap-x-4 sm:gap-x-6 flex-wrap sm:contents">
                <span className="flex items-center gap-2">
                  <IconPhone size={14} />
                  <span>+61 412954045</span>
                </span>
                <span className="hide-on-mobile sm:inline text-[var(--border-default)]">•</span>
                <span className="flex items-center gap-2">
                  <IconMapPin size={14} />
                  <span>Sydney, Australia</span>
                </span>
              </div>
            </div>

            {/* Action Buttons */}
            <div className="flex flex-col sm:flex-row justify-center items-center gap-3 pt-3 w-full sm:w-auto max-w-xs sm:max-w-none">
              <a
                id="btn-download-resume"
                href="/resume.pdf"
                download="Rahul_Dange_Resume.pdf"
                className="btn btn-primary btn-lg flex items-center justify-center gap-2 w-full sm:w-auto order-3 sm:order-1"
              >
                <IconDownload size={16} />
                Download Resume
              </a>
              <a
                id="btn-github"
                href="https://github.com/rahuldangeofficial"
                target="_blank"
                rel="noopener noreferrer"
                className="btn btn-secondary btn-lg flex items-center justify-center gap-2 w-full sm:w-auto order-2 sm:order-2"
              >
                <IconGithub size={16} />
                GitHub
              </a>
              <a
                id="btn-linkedin"
                href="https://www.linkedin.com/in/rahuldangeofficial/"
                target="_blank"
                rel="noopener noreferrer"
                className="btn btn-secondary btn-lg flex items-center justify-center gap-2 w-full sm:w-auto order-1 sm:order-3"
              >
                <IconLinkedin size={16} />
                LinkedIn
              </a>
            </div>

          </div>
        </div>
      </section>

      {/* Sticky Navigation */}
      <nav className="nav-bar sticky top-0 z-50">
        <div className="container">
          <div className="max-w-6xl mx-auto">
            {/* Desktop Navigation */}
            <div className="hidden lg:flex items-center justify-center py-3">
              <div className="flex items-center gap-1">
                {navigationItems.map((item) => (
                  <button
                    key={item.id}
                    id={`nav-${item.id}`}
                    onClick={() => scrollToSection(item.id)}
                    className={activeSection === item.id ? 'nav-link nav-link-active' : 'nav-link'}
                  >
                    {item.label}
                  </button>
                ))}
              </div>
            </div>

            {/* Mobile Navigation Bar */}
            <div className="lg:hidden flex items-center justify-between py-3 relative z-10">
              <div className="flex items-center gap-2">
                <span className="font-semibold text-sm sm:text-base text-[var(--fg-primary)]">Rahul Dange</span>
              </div>

              <button
                id="btn-mobile-menu-toggle"
                onClick={() => setIsMenuOpen(!isMenuOpen)}
                className="btn btn-ghost p-2"
                aria-label={isMenuOpen ? 'Close menu' : 'Open menu'}
              >
                {isMenuOpen ? <IconX size={20} /> : <IconMenu size={20} />}
              </button>
            </div>

            {/* Mobile Menu Panel */}
            {isMenuOpen && (
              <>
                <div className="mobile-menu-overlay lg:hidden" onClick={() => setIsMenuOpen(false)} />
                <div className="mobile-menu-panel lg:hidden">
                  <div className="px-4 py-2 space-y-1">
                    {navigationItems.map((item) => (
                      <button
                        key={item.id}
                        id={`nav-mobile-${item.id}`}
                        onClick={() => scrollToSection(item.id)}
                        className={`w-full text-left ${activeSection === item.id ? 'nav-link nav-link-active' : 'nav-link'}`}
                      >
                        {item.label}
                      </button>
                    ))}
                  </div>
                </div>
              </>
            )}
          </div>
        </div>
      </nav>
    </>
  );
});

Header.displayName = 'Header';

export default Header;