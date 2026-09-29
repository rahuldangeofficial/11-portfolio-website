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
  useEffect(() => {
    if (window.location.hash) {
      try {
        document.getElementById(decodeURIComponent(window.location.hash.slice(1)))?.scrollIntoView();
      } catch {
        // Ignore malformed percent escapes in incoming fragment links.
      }
    }
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