import React from 'react';

const PersonalInterests: React.FC = React.memo(() => {
  return (
    <section id="interests" className="section content-auto">
      <div className="container">
        <div className="max-w-4xl mx-auto">
          <h2 className="section-heading">Personal Interests &amp; Philosophy</h2>
          <p className="section-subheading">
            Beyond engineering, I am driven by deep curiosity, precise focus, and a desire to understand 
            the underlying systems of the world—whether in human behavior, nutrition, or physical geometry.
          </p>
          
          <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
            {/* Card 1: Asking the "Why" (Featured - spans 2 columns on tablet/desktop) */}
            <div className="card scroll-reveal md:col-span-2 lg:col-span-2 p-4 sm:p-6 md:p-8 flex flex-col justify-between group relative overflow-hidden border-[var(--accent-cyan)] shadow-glow-blue">
              {/* Decorative faint background element */}
              <div 
                className="absolute -right-4 -bottom-10 text-[180px] font-extrabold select-none pointer-events-none opacity-[0.02] font-mono leading-none transition-transform duration-700 group-hover:scale-110 group-hover:-rotate-12 text-[var(--accent-cyan)]"
              >
                ?
              </div>
              
              <div>
                <div className="flex items-center justify-between mb-4">
                  <div 
                    className="w-14 h-14 rounded-xl flex items-center justify-center transition-all group-hover:scale-110 duration-350 bg-[var(--accent-cyan-glow)] border border-[var(--accent-cyan)]"
                  >
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" className="text-[var(--accent-cyan)] w-6 h-6">
                      <circle cx="11" cy="11" r="8" />
                      <line x1="21" y1="21" x2="16.65" y2="16.65" />
                      <path d="M11 8a2.5 2.5 0 0 1 2 4.5 2 2 0 0 0-1.5 2" />
                      <line x1="11.5" y1="17.5" x2="11.5" y2="17.5" />
                    </svg>
                  </div>
                  <span className="text-[10px] font-bold uppercase tracking-wider px-2.5 py-0.5 rounded-full bg-[var(--accent-cyan-glow)] text-[var(--accent-cyan)] border border-[var(--accent-cyan)]">
                    Core Philosophy
                  </span>
                </div>
                
                <h3 className="text-xl sm:text-2xl font-bold mb-3 text-[var(--fg-primary)]">
                  Asking the "Why" Behind Everything
                </h3>
                <p className="text-sm sm:text-base leading-relaxed mb-6 text-[var(--fg-secondary)]">
                  First-principles analysis is the core driver of both my engineering career and daily life. 
                  I avoid taking abstractions at face value; instead, I seek to deconstruct systems to their 
                  fundamental assumptions. By questioning the "why," I can uncover root causes, strip away 
                  unnecessary complexity, and build more robust, future-proof software solutions.
                </p>
              </div>
              
              <div className="border-t border-[var(--border-subtle)] pt-4 mt-auto">
                <h4 className="text-xs font-semibold uppercase tracking-wider text-[var(--fg-muted)] mb-3">
                  Implementation in Work:
                </h4>
                <ul className="grid grid-cols-1 sm:grid-cols-2 gap-x-6 gap-y-2">
                  <li className="flex items-center gap-2">
                    <span className="w-1.5 h-1.5 rounded-full bg-[var(--border-default)] group-hover:bg-[var(--accent-cyan)] transition-colors flex-shrink-0" />
                    <span className="text-xs sm:text-sm text-[var(--fg-secondary)] group-hover:text-[var(--fg-primary)] transition-colors">First-principles system designs</span>
                  </li>
                  <li className="flex items-center gap-2">
                    <span className="w-1.5 h-1.5 rounded-full bg-[var(--border-default)] group-hover:bg-[var(--accent-cyan)] transition-colors flex-shrink-0" />
                    <span className="text-xs sm:text-sm text-[var(--fg-secondary)] group-hover:text-[var(--fg-primary)] transition-colors">Identifying and solving root causes</span>
                  </li>
                  <li className="flex items-center gap-2">
                    <span className="w-1.5 h-1.5 rounded-full bg-[var(--border-default)] group-hover:bg-[var(--accent-cyan)] transition-colors flex-shrink-0" />
                    <span className="text-xs sm:text-sm text-[var(--fg-secondary)] group-hover:text-[var(--fg-primary)] transition-colors">Refactoring complex technical debt</span>
                  </li>
                  <li className="flex items-center gap-2">
                    <span className="w-1.5 h-1.5 rounded-full bg-[var(--border-default)] group-hover:bg-[var(--accent-cyan)] transition-colors flex-shrink-0" />
                    <span className="text-xs sm:text-sm text-[var(--fg-secondary)] group-hover:text-[var(--fg-primary)] transition-colors">Avoiding cargo-cult engineering</span>
                  </li>
                </ul>
              </div>
            </div>

            {/* Card 2: Open Source Contributions */}
            <div className="card scroll-reveal p-4 sm:p-6 flex flex-col justify-between group relative overflow-hidden">
              <div>
                <div className="w-12 h-12 rounded-xl flex items-center justify-center mb-4 transition-transform group-hover:scale-110 duration-350 bg-[var(--bg-surface)] border border-[var(--border-subtle)]">
                  <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" className="text-[var(--fg-secondary)] group-hover:text-[var(--accent-cyan)] transition-colors w-5 h-5">
                    <polyline points="16 18 22 12 16 6" />
                    <polyline points="8 6 2 12 8 18" />
                    <path d="M12 5v14" />
                    <circle cx="12" cy="12" r="2" fill="currentColor" className="text-[var(--fg-secondary)] group-hover:text-[var(--accent-cyan)] transition-colors" />
                  </svg>
                </div>
                
                <h3 className="text-lg sm:text-xl font-bold mb-2 text-[var(--fg-primary)]">
                  Open Source Contributions
                </h3>
                <p className="text-xs sm:text-sm leading-relaxed mb-6 text-[var(--fg-secondary)]">
                  Active participation in the collective tools and packages that power the global software community. 
                  Maintaining packages and contributing upstream ensures high quality, review standards, and keeps my skills sharp.
                </p>
              </div>
              
              <div className="border-t border-[var(--border-subtle)] pt-4 mt-auto">
                <ul className="space-y-2">
                  <li className="flex items-center gap-2">
                    <span className="w-1.5 h-1.5 rounded-full bg-[var(--border-default)] group-hover:bg-[var(--accent-cyan)] transition-colors flex-shrink-0" />
                    <span className="text-xs sm:text-sm text-[var(--fg-secondary)] group-hover:text-[var(--fg-primary)] transition-colors">Upstream performance optimizations</span>
                  </li>
                  <li className="flex items-center gap-2">
                    <span className="w-1.5 h-1.5 rounded-full bg-[var(--border-default)] group-hover:bg-[var(--accent-cyan)] transition-colors flex-shrink-0" />
                    <span className="text-xs sm:text-sm text-[var(--fg-secondary)] group-hover:text-[var(--fg-primary)] transition-colors">Maintaining developer command-line utilities</span>
                  </li>
                </ul>
              </div>
            </div>

            {/* Card 3: Human Psychology */}
            <div className="card scroll-reveal p-4 sm:p-6 flex flex-col justify-between group relative overflow-hidden">
              <div>
                <div className="w-12 h-12 rounded-xl flex items-center justify-center mb-4 transition-transform group-hover:scale-110 duration-350 bg-[var(--bg-surface)] border border-[var(--border-subtle)]">
                  <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" className="text-[var(--fg-secondary)] group-hover:text-[var(--accent-cyan)] transition-colors w-5 h-5">
                    <path d="M12 5a3 3 0 1 0-5.997.125 4 4 0 0 0-2.526 5.77 4 4 0 0 0 .556 6.588A4 4 0 1 0 12 18Z" />
                    <path d="M12 5a3 3 0 1 1 5.997.125 4 4 0 0 1 2.526 5.77 4 4 0 0 1-.556 6.588A4 4 0 1 1 12 18Z" />
                    <path d="M12 5v14" />
                  </svg>
                </div>
                
                <h3 className="text-lg sm:text-xl font-bold mb-2 text-[var(--fg-primary)]">
                  Human Psychology
                </h3>
                <p className="text-xs sm:text-sm leading-relaxed mb-6 text-[var(--fg-secondary)]">
                  Software is constructed by people, run in teams, and used by end-users. Studying cognitive biases 
                  and behavior helps me design UX patterns that minimize friction and write team reviews that build 
                  empathy and psychological safety.
                </p>
              </div>
              
              <div className="border-t border-[var(--border-subtle)] pt-4 mt-auto">
                <ul className="space-y-2">
                  <li className="flex items-center gap-2">
                    <span className="w-1.5 h-1.5 rounded-full bg-[var(--border-default)] group-hover:bg-[var(--accent-cyan)] transition-colors flex-shrink-0" />
                    <span className="text-xs sm:text-sm text-[var(--fg-secondary)] group-hover:text-[var(--fg-primary)] transition-colors">Cognitive ergonomics in design</span>
                  </li>
                  <li className="flex items-center gap-2">
                    <span className="w-1.5 h-1.5 rounded-full bg-[var(--border-default)] group-hover:bg-[var(--accent-cyan)] transition-colors flex-shrink-0" />
                    <span className="text-xs sm:text-sm text-[var(--fg-secondary)] group-hover:text-[var(--fg-primary)] transition-colors">Healthy engineering team communication</span>
                  </li>
                </ul>
              </div>
            </div>

            {/* Card 4: Food Nutrition */}
            <div className="card scroll-reveal p-4 sm:p-6 flex flex-col justify-between group relative overflow-hidden">
              <div>
                <div className="w-12 h-12 rounded-xl flex items-center justify-center mb-4 transition-transform group-hover:scale-110 duration-350 bg-[var(--bg-surface)] border border-[var(--border-subtle)]">
                  <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" className="text-[var(--fg-secondary)] group-hover:text-[var(--accent-cyan)] transition-colors w-5 h-5">
                    <path d="M12 22c4.97 0 9-3.03 9-8 0-4.08-2.51-7.24-5-8.5-1.02-.51-2.48-.5-3.5.5L12 7l-.5-.5c-1.02-1-2.48-1.01-3.5-.5C5.51 7.26 3 10.42 3 14c0 4.97 4.03 8 9 8z" />
                    <path d="M12 5V2" />
                    <path d="M15 3c-1 1-2.5 1-2.5 1" />
                  </svg>
                </div>
                
                <h3 className="text-lg sm:text-xl font-bold mb-2 text-[var(--fg-primary)]">
                  Food Nutrition
                </h3>
                <p className="text-xs sm:text-sm leading-relaxed mb-6 text-[var(--fg-secondary)]">
                  Maintaining mental stamina is critical for deep work. I read about nutritional biochemistry and 
                  metabolic physiology to structure habits, control energy release levels, and maintain sharp cognitive focus.
                </p>
              </div>
              
              <div className="border-t border-[var(--border-subtle)] pt-4 mt-auto">
                <ul className="space-y-2">
                  <li className="flex items-center gap-2">
                    <span className="w-1.5 h-1.5 rounded-full bg-[var(--border-default)] group-hover:bg-[var(--accent-cyan)] transition-colors flex-shrink-0" />
                    <span className="text-xs sm:text-sm text-[var(--fg-secondary)] group-hover:text-[var(--fg-primary)] transition-colors">Study of metabolic efficiency and nutrition</span>
                  </li>
                  <li className="flex items-center gap-2">
                    <span className="w-1.5 h-1.5 rounded-full bg-[var(--border-default)] group-hover:bg-[var(--accent-cyan)] transition-colors flex-shrink-0" />
                    <span className="text-xs sm:text-sm text-[var(--fg-secondary)] group-hover:text-[var(--fg-primary)] transition-colors">Sustaining peak energy and mental clarity</span>
                  </li>
                </ul>
              </div>
            </div>

            {/* Card 5: Teaching */}
            <div className="card scroll-reveal p-4 sm:p-6 flex flex-col justify-between group relative overflow-hidden">
              <div>
                <div className="w-12 h-12 rounded-xl flex items-center justify-center mb-4 transition-transform group-hover:scale-110 duration-350 bg-[var(--bg-surface)] border border-[var(--border-subtle)]">
                  <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" className="text-[var(--fg-secondary)] group-hover:text-[var(--accent-cyan)] transition-colors w-5 h-5">
                    <rect x="2" y="3" width="20" height="14" rx="2" />
                    <path d="M8 21h8" />
                    <path d="M12 17v4" />
                    <path d="M7 8l3 3 7-7" />
                  </svg>
                </div>
                
                <h3 className="text-lg sm:text-xl font-bold mb-2 text-[var(--fg-primary)]">
                  Teaching &amp; Mentorship
                </h3>
                <p className="text-xs sm:text-sm leading-relaxed mb-6 text-[var(--fg-secondary)]">
                  Simplifying advanced computational concepts is the ultimate test of understanding. I enjoy teaching, 
                  breaking down architecture patterns into intuitive mental models, and writing clear developer guides.
                </p>
              </div>
              
              <div className="border-t border-[var(--border-subtle)] pt-4 mt-auto">
                <ul className="space-y-2">
                  <li className="flex items-center gap-2">
                    <span className="w-1.5 h-1.5 rounded-full bg-[var(--border-default)] group-hover:bg-[var(--accent-cyan)] transition-colors flex-shrink-0" />
                    <span className="text-xs sm:text-sm text-[var(--fg-secondary)] group-hover:text-[var(--fg-primary)] transition-colors">Mentoring student and junior engineers</span>
                  </li>
                  <li className="flex items-center gap-2">
                    <span className="w-1.5 h-1.5 rounded-full bg-[var(--border-default)] group-hover:bg-[var(--accent-cyan)] transition-colors flex-shrink-0" />
                    <span className="text-xs sm:text-sm text-[var(--fg-secondary)] group-hover:text-[var(--fg-primary)] transition-colors">Authoring architectural mental models</span>
                  </li>
                </ul>
              </div>
            </div>

            {/* Card 6: Miniature Paper Engineering (Full-width Bento item) */}
            <div className="card scroll-reveal lg:col-span-3 md:col-span-2 p-4 sm:p-6 md:p-8 flex flex-col md:flex-row gap-6 md:gap-8 items-center justify-between group relative overflow-hidden">
              {/* Blueprint background grid pattern on the right */}
              <div className="absolute right-0 top-0 bottom-0 w-1/3 opacity-[0.03] pointer-events-none hidden md:block select-none">
                <svg className="w-full h-full text-[var(--accent-cyan)]" viewBox="0 0 100 100" preserveAspectRatio="none">
                  <path d="M0 0 L100 100 M100 0 L0 100 M50 0 V100 M0 50 H100" stroke="currentColor" strokeWidth="0.5" strokeDasharray="1,2" fill="none" />
                  <circle cx="50" cy="50" r="20" stroke="currentColor" strokeWidth="0.5" strokeDasharray="1,2" fill="none" />
                  <polygon points="50,10 90,50 50,90 10,50" stroke="currentColor" strokeWidth="0.5" strokeDasharray="1,2" fill="none" />
                </svg>
              </div>

              <div className="flex-1 text-left">
                <div className="flex items-center gap-3.5 mb-4">
                  <div className="w-12 h-12 rounded-xl flex items-center justify-center transition-transform group-hover:scale-110 duration-350 bg-[var(--bg-surface)] border border-[var(--border-subtle)]">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" className="text-[var(--fg-secondary)] group-hover:text-[var(--accent-cyan)] transition-colors w-5 h-5">
                      <circle cx="6" cy="6" r="3" />
                      <circle cx="6" cy="18" r="3" />
                      <line x1="9.8" y1="8.2" x2="20" y2="18" />
                      <line x1="9.8" y1="15.8" x2="20" y2="6" />
                    </svg>
                  </div>
                  <span className="text-[10px] font-bold uppercase tracking-wider px-2.5 py-0.5 rounded-full bg-[var(--bg-surface)] text-[var(--fg-secondary)] border border-[var(--border-default)]">
                    Micro-Scale Crafting
                  </span>
                </div>
                
                <h3 className="text-xl sm:text-2xl font-bold mb-3 text-[var(--fg-primary)]">
                  Miniature Paper Engineering
                </h3>
                <p className="text-sm sm:text-base leading-relaxed mb-6 max-w-3xl text-[var(--fg-secondary)]">
                  A screen-free, highly tactical creative outlet requiring extreme focus and steady hands. I design 
                  and construct precise miniature structures and mechanisms using only paper, pens, scissors, and adhesive (gum). 
                  This hobby exercises micro-scale patience, structural logic, and millimeter-level physical precision.
                </p>
                
                <div className="border-t border-[var(--border-subtle)] pt-4">
                  <ul className="grid grid-cols-1 sm:grid-cols-3 gap-3">
                    <li className="flex items-center gap-2">
                      <span className="w-1.5 h-1.5 rounded-full bg-[var(--border-default)] group-hover:bg-[var(--accent-cyan)] transition-colors flex-shrink-0" />
                      <span className="text-xs sm:text-sm text-[var(--fg-secondary)] group-hover:text-[var(--fg-primary)] transition-colors">Precise cuts & scissor control</span>
                    </li>
                    <li className="flex items-center gap-2">
                      <span className="w-1.5 h-1.5 rounded-full bg-[var(--border-default)] group-hover:bg-[var(--accent-cyan)] transition-colors flex-shrink-0" />
                      <span className="text-xs sm:text-sm text-[var(--fg-secondary)] group-hover:text-[var(--fg-primary)] transition-colors">Adhesive & gum assembly detail</span>
                    </li>
                    <li className="flex items-center gap-2">
                      <span className="w-1.5 h-1.5 rounded-full bg-[var(--border-default)] group-hover:bg-[var(--accent-cyan)] transition-colors flex-shrink-0" />
                      <span className="text-xs sm:text-sm text-[var(--fg-secondary)] group-hover:text-[var(--fg-primary)] transition-colors">Drafting with pens & rulers</span>
                    </li>
                  </ul>
                </div>
              </div>

              {/* Animated cutting mat and model vector */}
              <div className="flex-shrink-0 w-36 h-36 hidden md:flex items-center justify-center relative bg-[var(--bg-root)] border border-[var(--border-subtle)] rounded-2xl p-4 transition-all duration-500 group-hover:border-[var(--accent-cyan)] group-hover:shadow-sm">
                <svg viewBox="0 0 100 100" className="w-24 h-24 text-[var(--fg-secondary)] group-hover:text-[var(--accent-cyan)] transition-all duration-500 transform group-hover:scale-105">
                  {/* Cutting mat grid */}
                  <rect x="5" y="5" width="90" height="90" rx="4" stroke="currentColor" strokeWidth="0.8" strokeDasharray="1,2" fill="none" opacity="0.3" />
                  <line x1="25" y1="5" x2="25" y2="95" stroke="currentColor" strokeWidth="0.5" strokeDasharray="1,4" opacity="0.2" />
                  <line x1="50" y1="5" x2="50" y2="95" stroke="currentColor" strokeWidth="0.5" strokeDasharray="1,4" opacity="0.2" />
                  <line x1="75" y1="5" x2="75" y2="95" stroke="currentColor" strokeWidth="0.5" strokeDasharray="1,4" opacity="0.2" />
                  <line x1="5" y1="25" x2="95" y2="25" stroke="currentColor" strokeWidth="0.5" strokeDasharray="1,4" opacity="0.2" />
                  <line x1="5" y1="50" x2="95" y2="50" stroke="currentColor" strokeWidth="0.5" strokeDasharray="1,4" opacity="0.2" />
                  <line x1="5" y1="75" x2="95" y2="75" stroke="currentColor" strokeWidth="0.5" strokeDasharray="1,4" opacity="0.2" />

                  {/* Ruler */}
                  <rect x="15" y="80" width="70" height="8" rx="1" stroke="currentColor" strokeWidth="1" fill="none" />
                  <line x1="25" y1="80" x2="25" y2="84" stroke="currentColor" strokeWidth="0.8" />
                  <line x1="35" y1="80" x2="35" y2="84" stroke="currentColor" strokeWidth="0.8" />
                  <line x1="45" y1="80" x2="45" y2="84" stroke="currentColor" strokeWidth="0.8" />
                  <line x1="55" y1="80" x2="55" y2="84" stroke="currentColor" strokeWidth="0.8" />
                  <line x1="65" y1="80" x2="65" y2="84" stroke="currentColor" strokeWidth="0.8" />
                  <line x1="75" y1="80" x2="75" y2="84" stroke="currentColor" strokeWidth="0.8" />

                  {/* Isometric structure block */}
                  <polygon points="50,20 80,35 80,65 50,80 20,65 20,35" stroke="currentColor" strokeWidth="1.2" fill="none" />
                  <polygon points="50,20 50,80" stroke="currentColor" strokeWidth="0.8" />
                  <line x1="20" y1="35" x2="50" y2="50" stroke="currentColor" strokeWidth="0.8" />
                  <line x1="80" y1="35" x2="50" y2="50" stroke="currentColor" strokeWidth="0.8" />
                  <line x1="20" y1="65" x2="50" y2="50" stroke="currentColor" strokeWidth="0.8" strokeDasharray="1,1" opacity="0.6" />
                  <line x1="80" y1="65" x2="50" y2="50" stroke="currentColor" strokeWidth="0.8" strokeDasharray="1,1" opacity="0.6" />

                  {/* Dotted cutting outline */}
                  <path d="M12,12 L88,12 L88,88 L12,88 Z" stroke="currentColor" strokeWidth="1" strokeDasharray="2,2" fill="none" opacity="0.4" />
                </svg>
              </div>
            </div>
          </div>
        </div>
      </div>
    </section>
  );
});

PersonalInterests.displayName = 'PersonalInterests';

export default PersonalInterests;