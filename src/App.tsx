import { useEffect } from 'react';
import Header from './components/Header';
import About from './components/About';
import Projects from './components/Projects';
import Experience from './components/Experience';
import Skills from './components/Skills';
import Education from './components/Education';
import Publications from './components/Publications';
import Contact from './components/Contact';

function App() {
  // Simple IntersectionObserver for smooth entrance without scroll hijacking
  useEffect(() => {
    // Force start at the very top of the page on load
    if ('scrollRestoration' in window.history) {
      window.history.scrollRestoration = 'manual';
    }
    window.scrollTo(0, 0);

    document.documentElement.classList.add('js-enabled');

    const observer = new IntersectionObserver(
      (entries) => {
        entries.forEach((entry) => {
          if (entry.isIntersecting) {
            entry.target.classList.add('is-visible');
          }
        });
      },
      { threshold: 0.05, rootMargin: '0px 0px -20px 0px' }
    );

    const targets = document.querySelectorAll('.scroll-reveal');
    targets.forEach((el) => observer.observe(el));

    return () => {
      observer.disconnect();
      document.documentElement.classList.remove('js-enabled');
    };
  }, []);

  return (
    <div className="min-h-screen bg-[var(--bg-root)] text-[var(--fg-primary)] selection:bg-[var(--fg-primary)] selection:text-[var(--bg-primary)]">
      <Header />
      <main className="max-w-4xl mx-auto px-5 sm:px-8 lg:px-8 py-8 space-y-16 sm:space-y-20">
        <About />
        <Projects />
        <Experience />
        <Skills />
        <Education />
        <Publications />
        <Contact />
      </main>
    </div>
  );
}

export default App;