import React from 'react';

const About: React.FC = React.memo(() => {
  return (
    <section id="about" className="section">
      <div className="container">
        <div className="max-w-4xl mx-auto">
          <h2 className="section-heading mb-8 md:mb-12">About Me</h2>

          <div className="card scroll-reveal p-4 sm:p-6 md:p-8">
            <div className="space-y-4">
              <p className="leading-relaxed text-sm sm:text-base text-[var(--fg-secondary)]">
                I am engineer who solves problems, real engineering problems, because I can see them in multiple dimensions. Every problem I encounter is a mechanism that forces my brain to expand. I systematically tear complexity apart, craft the necessary pieces of the solution, and fuse them together.
              </p>

              <p className="leading-relaxed text-sm sm:text-base text-[var(--fg-secondary)]">
                If you ask me how I know I can do this, it's because I have failed more times than I can count. But every single failure gave me the exact why and the what that I was missing. I am not driven by being the 'best' engineer in the room, I am driven by the absolute certainty that no matter the complexity, I will figure it out.
              </p>

              <p className="leading-relaxed text-sm sm:text-base text-[var(--fg-secondary)]">
                Beyond the code, my greatest asset is the ability to teach. My proof? I taught the most challenging student on the planet, myself. I took someone who felt entirely out of his depth and forged him into an engineer who genuinely believes he can solve anything.
              </p>

              <p className="leading-relaxed text-sm sm:text-base text-[var(--fg-secondary)]">
                That is exactly what I bring to the table.
              </p>
            </div>
          </div>
        </div>
      </div>
    </section>
  );
});

About.displayName = 'About';

export default About;