import React, { useEffect } from 'react';
import Lenis from 'lenis';
import Header from './components/Header';
import About from './components/About';
import Experience from './components/Experience';
import Skills from './components/Skills';
import Projects from './components/Projects';
import Education from './components/Education';
import TechnicalPreferences from './components/TechnicalPreferences';
import PersonalInterests from './components/PersonalInterests';
import Publications from './components/Publications';
import Volunteering from './components/Volunteering';
import MyStory from './components/MyStory';
import Contact from './components/Contact';

function App() {
  // Initialize Lenis Smooth Scroll
  useEffect(() => {
    // Detect native Safari / Apple WebKit browsers to bypass custom smooth scrolling.
    // Safari's native engine handles momentum scrolling, trackpads, and ProMotion displays natively.
    const isSafari =
      (navigator.vendor && navigator.vendor.includes('Apple')) ||
      (/Safari/.test(navigator.userAgent) && !/Chrome/.test(navigator.userAgent) && !/Chromium/.test(navigator.userAgent));

    if (isSafari) {
      document.documentElement.classList.add('safari-native');
      return;
    }
    document.documentElement.classList.remove('safari-native');

    const lenis = new Lenis({
      lerp: 0.22, // Default linear interpolation for mouse wheels (low inertia, highly responsive)
      orientation: 'vertical',
      gestureOrientation: 'vertical',
      smoothWheel: true,
      wheelMultiplier: 1.4, // Boosted travel multiplier for a fast, "super lightweight" scroll feel
      touchMultiplier: 1.5,
      infinite: false,
      autoRaf: true,
    });

    // Share Lenis globally for custom nav transitions
    (window as any).lenis = lenis;

    // Dynamic trackpad vs. mouse wheel scroll interpolation adaptation.
    // Trackpads output high-frequency scroll events with fractional or small delta values.
    // Traditional mouse wheels output discrete integer steps (typically multiples of 30, 40, or 120).
    const handleWheel = (e: WheelEvent) => {
      const isFractional = e.deltaY % 1 !== 0;
      const isSmallDelta = Math.abs(e.deltaY) < 15;

      if (isFractional || isSmallDelta) {
        lenis.lerp = 0.80; // Trackpad: Instant native feedback (zero artificial lag/inertia)
      } else {
        lenis.lerp = 0.22; // Mouse: Snappy catch-up with smooth deceleration
      }
    };

    window.addEventListener('wheel', handleWheel, { passive: true });

    return () => {
      lenis.destroy();
      (window as any).lenis = null;
      window.removeEventListener('wheel', handleWheel);
    };
  }, []);

  // IntersectionObserver for scroll-reveal animations
  useEffect(() => {
    // Add js-enabled class to html element to activate hidden opacity state safely
    document.documentElement.classList.add('js-enabled');

    const observer = new IntersectionObserver(
      (entries) => {
        entries.forEach((entry) => {
          if (entry.isIntersecting) {
            entry.target.classList.add('is-visible');
          }
        });
      },
      { threshold: 0.05, rootMargin: '0px 0px -40px 0px' }
    );

    // Observe all scroll-reveal elements
    const targets = document.querySelectorAll('.scroll-reveal');
    targets.forEach((el) => observer.observe(el));

    return () => {
      observer.disconnect();
      document.documentElement.classList.remove('js-enabled');
    };
  }, []);

  return (
    <div className="min-h-screen">
      <Header />
      <main>
        <About />
        <Experience />
        <Skills />
        <Projects />
        <Education />
        <TechnicalPreferences />
        <PersonalInterests />
        <Publications />
        <Volunteering />
        <MyStory />
        <Contact />
      </main>
    </div>
  );
}

export default App;