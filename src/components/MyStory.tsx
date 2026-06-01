import React from 'react';
import { IconTimeline } from './Icons';

const MyStory: React.FC = React.memo(() => {
  return (
    <section id="mystory" className="section section-alt content-auto">
      <div className="container">
        <div className="max-w-4xl mx-auto">
          <article className="card scroll-reveal p-4 sm:p-6 md:p-8">
            <div className="flex flex-col sm:flex-row sm:items-center gap-3 mb-4 sm:mb-6">
              <div 
                className="w-12 h-12 rounded-full flex items-center justify-center flex-shrink-0 mx-0 bg-[var(--accent-cyan-glow)] border border-[var(--accent-cyan)]"
              >
                <IconTimeline size={20} className="text-[var(--accent-cyan)]" />
              </div>
              <h3 
                className="text-xl sm:text-2xl font-semibold m-0 text-left text-[var(--fg-primary)]"
              >
                My Story: The Architecture of a Mind
              </h3>
            </div>
            
            <p className="leading-relaxed mb-4 sm:mb-6 text-sm sm:text-base text-left text-[var(--fg-secondary)]">
              I still remember opening a computer for the first time. Most people look at a screen and see magic, but I looked at it and saw a system I didn't understand yet.
            </p>
            
            <p className="leading-relaxed mb-4 sm:mb-6 text-sm sm:text-base text-left text-[var(--fg-secondary)]">
              If you had met me then, you wouldn't have seen a prodigy. The truth is, I was never naturally smart. My entire life has been a sequence of aggressive, complex transitions. I started my education in a Marathi medium, shifted to a Hindi medium, and then forced myself to adapt to English. Eventually, I pushed myself onto the international stage, navigating an entirely new country, a new culture, and completely new classes of problems.
            </p>
            
            <p className="leading-relaxed mb-4 sm:mb-6 text-sm sm:text-base text-left text-[var(--fg-secondary)]">
              Every single one of those jumps was a massive leap in complexity. I didn't survive those transitions by being the smartest person in the room. I survived by treating my environment like a system. I broke it down, isolated the new rules, adapted, and compiled a solution.
            </p>
            
            <p className="leading-relaxed mb-4 sm:mb-6 text-sm sm:text-base text-left text-[var(--fg-secondary)]">
              By the time I finished my Master's degree, my perspective was already locked in. I wasn't suddenly awakened or shocked by the real world. The intense curiosity about how things work at their absolute lowest layers was already burning inside me. Graduation didn't give me a new mindset, it gave me the time. It gave me the freedom to finally execute, to dive straight to the bare metal, and to build the things I had been obsessing over.
            </p>
            
            <p className="leading-relaxed mb-4 sm:mb-6 text-sm sm:text-base text-left text-[var(--fg-secondary)]">
              I didn't become a systems engineer by accident. I treated my own brain like infrastructure that needed to be optimized. When I hit a wall, I didn't quit, I read the error logs of my own failures. I isolated the exact flaw in my logic, I patched it, and I iterated. I took a blank, unpromising slate and relentlessly programmed him into a problem-solver who absolutely believes that no system is too complex to be deconstructed.
            </p>
            
            <p className="leading-relaxed mb-4 sm:mb-6 text-sm sm:text-base text-left text-[var(--fg-secondary)] font-medium">
              I wasn't born an engineer. I engineered myself.
            </p>
            
            <p className="leading-relaxed text-sm sm:text-base text-left text-[var(--fg-secondary)]">
              I am Rahul Dange, and that is exactly how I approach every problem in my path.
            </p>
          </article>
        </div>
      </div>
    </section>
  );
});

MyStory.displayName = 'MyStory';

export default MyStory;
