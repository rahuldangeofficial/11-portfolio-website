import React from 'react';
import {
  IconMail,
  IconPhone,
  IconMapPin,
  IconGithub,
  IconLinkedin,
  IconGlobe,
  IconMessageCircle,
  IconChevronRight,
} from './Icons';

const Contact: React.FC = React.memo(() => {
  return (
    <section id="contact" className="section content-auto">
      <div className="container">
        <div className="max-w-4xl mx-auto">
          <h2 className="section-heading">Let's Connect</h2>
          <p className="section-subheading">
            I am always interested in discussing new projects, distributed systems design, performance optimizations, or product architectures.
          </p>

          <div className="grid grid-cols-1 md:grid-cols-2 gap-6 md:gap-8">
            {/* Get In Touch */}
            <div className="card p-4 sm:p-6 md:p-8 flex flex-col justify-between">
              <div>
                <h3
                  className="text-lg sm:text-xl font-semibold mb-6 flex items-center gap-2 text-[var(--fg-primary)]"
                >
                  <IconMessageCircle size={20} className="text-[var(--accent-cyan)]" />
                  Get In Touch
                </h3>
                <div className="space-y-6">
                  {/* Email */}
                  <a
                    href="mailto:rahuldangeofficial@gmail.com"
                    className="flex items-start gap-4 group cursor-pointer text-left"
                  >
                    <div
                      className="w-12 h-12 rounded-full flex items-center justify-center flex-shrink-0 transition-colors group-hover:bg-[var(--accent-cyan-glow)] bg-[var(--bg-surface)] border border-[var(--border-subtle)]"
                    >
                      <IconMail size={18} className="text-[var(--fg-secondary)] group-hover:text-[var(--accent-cyan)] transition-colors" />
                    </div>
                    <div className="min-w-0 flex-1">
                      <p className="text-xs font-semibold uppercase tracking-wider text-[var(--fg-muted)] mb-0.5">
                        Email
                      </p>
                      <p
                        className="font-medium text-sm sm:text-base break-all transition-colors group-hover:text-[var(--accent-cyan)] text-[var(--fg-primary)]"
                      >
                        rahuldangeofficial@gmail.com
                      </p>
                      <p className="text-xs mt-0.5 text-[var(--fg-faint)]">
                        Best for detailed discussions
                      </p>
                    </div>
                  </a>

                  {/* Phone */}
                  <a
                    href="tel:+61412954045"
                    className="flex items-start gap-4 group cursor-pointer text-left"
                  >
                    <div
                      className="w-12 h-12 rounded-full flex items-center justify-center flex-shrink-0 transition-colors group-hover:bg-[var(--accent-cyan-glow)] bg-[var(--bg-surface)] border border-[var(--border-subtle)]"
                    >
                      <IconPhone size={18} className="text-[var(--fg-secondary)] group-hover:text-[var(--accent-cyan)] transition-colors" />
                    </div>
                    <div className="min-w-0 flex-1">
                      <p className="text-xs font-semibold uppercase tracking-wider text-[var(--fg-muted)] mb-0.5">
                        Phone
                      </p>
                      <p
                        className="font-medium text-sm sm:text-base transition-colors group-hover:text-[var(--accent-cyan)] text-[var(--fg-primary)]"
                      >
                        +61 412954045
                      </p>
                      <p className="text-xs mt-0.5 text-[var(--fg-faint)]">
                        Available 9 AM - 6 PM AEST
                      </p>
                    </div>
                  </a>

                  {/* Location */}
                  <a
                    href="https://maps.google.com/?q=Sydney,+Australia"
                    target="_blank"
                    rel="noopener noreferrer"
                    className="flex items-start gap-4 group cursor-pointer text-left"
                  >
                    <div
                      className="w-12 h-12 rounded-full flex items-center justify-center flex-shrink-0 transition-colors group-hover:bg-[var(--accent-cyan-glow)] bg-[var(--bg-surface)] border border-[var(--border-subtle)]"
                    >
                      <IconMapPin size={18} className="text-[var(--fg-secondary)] group-hover:text-[var(--accent-cyan)] transition-colors" />
                    </div>
                    <div className="min-w-0 flex-1">
                      <p className="text-xs font-semibold uppercase tracking-wider text-[var(--fg-muted)] mb-0.5">
                        Location
                      </p>
                      <p
                        className="font-medium text-sm sm:text-base transition-colors group-hover:text-[var(--accent-cyan)] text-[var(--fg-primary)]"
                      >
                        Sydney, Australia
                      </p>
                      <p className="text-xs mt-0.5 text-[var(--fg-faint)]">
                        Open to remote opportunities
                      </p>
                    </div>
                  </a>
                </div>
              </div>
            </div>

            {/* Professional Links */}
            <div className="card p-4 sm:p-6 md:p-8 flex flex-col justify-between">
              <div>
                <h3
                  className="text-lg sm:text-xl font-semibold mb-6 text-[var(--fg-primary)] text-left"
                >
                  Professional Links
                </h3>
                <div className="space-y-4">
                  {/* Website */}
                  <a
                    id="contact-website"
                    href="https://www.rahuldange.com"
                    target="_blank"
                    rel="noopener noreferrer"
                    className="group flex items-center justify-between p-4 rounded-xl border border-[var(--border-subtle)] bg-[var(--bg-surface)] hover:bg-[var(--bg-surface-hover)] hover:border-[var(--accent-cyan)] hover:shadow-[0_0_15px_rgba(24,119,242,0.06)] hover:scale-[1.01] transition-all duration-300 ease-[cubic-bezier(0.25,0.8,0.25,1)]"
                  >
                    <div className="flex items-center gap-3.5 min-w-0">
                      <IconGlobe size={22} className="text-[var(--fg-secondary)] group-hover:text-[var(--accent-cyan)] transition-colors flex-shrink-0" />
                      <div className="text-left min-w-0">
                        <div className="font-semibold text-sm sm:text-base text-[var(--fg-primary)]">Website</div>
                        <div className="text-xs text-[var(--fg-muted)] truncate">
                          rahuldange.com
                        </div>
                      </div>
                    </div>
                    <IconChevronRight size={16} className="text-[var(--fg-faint)] group-hover:text-[var(--accent-cyan)] group-hover:translate-x-1 transition-all duration-300 ease-[cubic-bezier(0.25,0.8,0.25,1)] flex-shrink-0" />
                  </a>

                  {/* GitHub */}
                  <a
                    id="contact-github"
                    href="https://github.com/rahuldangeofficial"
                    target="_blank"
                    rel="noopener noreferrer"
                    className="group flex items-center justify-between p-4 rounded-xl border border-[var(--border-subtle)] bg-[var(--bg-surface)] hover:bg-[var(--bg-surface-hover)] hover:border-[var(--accent-cyan)] hover:shadow-[0_0_15px_rgba(24,119,242,0.06)] hover:scale-[1.01] transition-all duration-300 ease-[cubic-bezier(0.25,0.8,0.25,1)]"
                  >
                    <div className="flex items-center gap-3.5 min-w-0">
                      <IconGithub size={22} className="text-[var(--fg-secondary)] group-hover:text-[var(--accent-cyan)] transition-colors flex-shrink-0" />
                      <div className="text-left min-w-0">
                        <div className="font-semibold text-sm sm:text-base text-[var(--fg-primary)]">GitHub</div>
                        <div className="text-xs text-[var(--fg-muted)] truncate">
                          View my code repositories
                        </div>
                      </div>
                    </div>
                    <IconChevronRight size={16} className="text-[var(--fg-faint)] group-hover:text-[var(--accent-cyan)] group-hover:translate-x-1 transition-all duration-300 ease-[cubic-bezier(0.25,0.8,0.25,1)] flex-shrink-0" />
                  </a>

                  {/* LinkedIn */}
                  <a
                    id="contact-linkedin"
                    href="https://www.linkedin.com/in/rahuldangeofficial/"
                    target="_blank"
                    rel="noopener noreferrer"
                    className="group flex items-center justify-between p-4 rounded-xl border border-[var(--border-subtle)] bg-[var(--bg-surface)] hover:bg-[var(--bg-surface-hover)] hover:border-[var(--accent-cyan)] hover:shadow-[0_0_15px_rgba(24,119,242,0.06)] hover:scale-[1.01] transition-all duration-300 ease-[cubic-bezier(0.25,0.8,0.25,1)]"
                  >
                    <div className="flex items-center gap-3.5 min-w-0">
                      <IconLinkedin size={22} className="text-[var(--fg-secondary)] group-hover:text-[var(--accent-cyan)] transition-colors flex-shrink-0" />
                      <div className="text-left min-w-0">
                        <div className="font-semibold text-sm sm:text-base text-[var(--fg-primary)]">LinkedIn</div>
                        <div className="text-xs text-[var(--fg-muted)] truncate">
                          Professional network &amp; experience
                        </div>
                      </div>
                    </div>
                    <IconChevronRight size={16} className="text-[var(--fg-faint)] group-hover:text-[var(--accent-cyan)] group-hover:translate-x-1 transition-all duration-300 ease-[cubic-bezier(0.25,0.8,0.25,1)] flex-shrink-0" />
                  </a>
                </div>
              </div>

              <div
                className="mt-6 pt-5 border-t border-[var(--border-subtle)] text-left"
              >
                <h4
                  className="font-semibold mb-2 text-sm sm:text-base text-[var(--fg-primary)]"
                >
                  Response Time
                </h4>
                <p
                  className="text-xs sm:text-sm leading-relaxed text-[var(--fg-secondary)]"
                >
                  I typically respond to emails within 24 hours. For urgent matters, feel free to reach out via phone or LinkedIn.
                </p>
              </div>
            </div>
          </div>

          {/* Footer */}
          <footer className="mt-16 md:mt-24 pt-8 md:pt-12 pb-6 border-t border-[var(--border-subtle)]">
            <div className="flex flex-col md:flex-row md:justify-between items-center md:items-start gap-8 text-center md:text-left mb-8 md:mb-12">
              <div className="max-w-md space-y-3">
                <p className="text-base font-bold text-[var(--fg-primary)] tracking-tight">
                  Rahul Dange
                </p>
                <p className="text-sm leading-relaxed text-[var(--fg-secondary)] max-w-sm md:max-w-md mx-auto md:mx-0">
                  Computer Engineer
                  <span className="hidden md:inline"> | </span>
                  <span className="block md:inline">Systems, Algorithms &amp; Product Architecture</span>
                </p>
              </div>
              
              <div className="flex justify-center md:justify-start gap-x-16 sm:gap-x-20 gap-y-6 w-full md:w-auto px-4 md:px-0">
                <div className="text-left w-32 sm:w-auto">
                  <h4 className="text-xs uppercase font-bold tracking-wider text-[var(--fg-primary)] mb-3">Navigation</h4>
                  <ul className="space-y-2 text-sm">
                    <li><a href="#about" className="text-[var(--fg-muted)] hover:text-[var(--accent-cyan)] transition-colors">About</a></li>
                    <li><a href="#experience" className="text-[var(--fg-muted)] hover:text-[var(--accent-cyan)] transition-colors">Experience</a></li>
                    <li><a href="#skills" className="text-[var(--fg-muted)] hover:text-[var(--accent-cyan)] transition-colors">Skills</a></li>
                    <li><a href="#projects" className="text-[var(--fg-muted)] hover:text-[var(--accent-cyan)] transition-colors">Projects</a></li>
                    <li><a href="#preferences" className="text-[var(--fg-muted)] hover:text-[var(--accent-cyan)] transition-colors">Tech Stack</a></li>
                  </ul>
                </div>
                <div className="text-left w-32 sm:w-auto">
                  <h4 className="text-xs uppercase font-bold tracking-wider text-[var(--fg-primary)] mb-3">Directory</h4>
                  <ul className="space-y-2 text-sm">
                    <li><a href="#education" className="text-[var(--fg-muted)] hover:text-[var(--accent-cyan)] transition-colors">Education</a></li>
                    <li><a href="#mystory" className="text-[var(--fg-muted)] hover:text-[var(--accent-cyan)] transition-colors">My Story</a></li>
                    <li><a href="#publications" className="text-[var(--fg-muted)] hover:text-[var(--accent-cyan)] transition-colors">Publications</a></li>
                    <li><a href="#volunteering" className="text-[var(--fg-muted)] hover:text-[var(--accent-cyan)] transition-colors">Volunteering</a></li>
                  </ul>
                </div>
              </div>
            </div>

            <div className="flex flex-col sm:flex-row items-center justify-between gap-4 pt-6 border-t border-[var(--border-subtle)] font-mono text-xs text-[var(--fg-faint)]">
              <p className="order-2 sm:order-1 text-center sm:text-left">
                © {new Date().getFullYear()} Rahul Dange. All rights reserved.
              </p>
              <div className="flex items-center justify-center gap-4 order-1 sm:order-2">
                <a 
                  href="https://github.com/rahuldangeofficial" 
                  target="_blank" 
                  rel="noopener noreferrer" 
                  className="text-[var(--fg-secondary)] hover:text-[var(--accent-cyan)] transition-colors"
                  aria-label="GitHub"
                >
                  <IconGithub size={18} />
                </a>
                <a 
                  href="https://www.linkedin.com/in/rahuldangeofficial/" 
                  target="_blank" 
                  rel="noopener noreferrer" 
                  className="text-[var(--fg-secondary)] hover:text-[var(--accent-cyan)] transition-colors"
                  aria-label="LinkedIn"
                >
                  <IconLinkedin size={18} />
                </a>
              </div>
            </div>
          </footer>
        </div>
      </div>
    </section>
  );
});

Contact.displayName = 'Contact';

export default Contact;