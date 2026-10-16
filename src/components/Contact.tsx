import React from 'react';
import { IconMail, IconGithub, IconLinkedin, IconDownload } from './Icons';

const Contact: React.FC = React.memo(() => {
  return (
    <section id="contact" className="space-y-6">
      <div className="pb-2 border-b border-[var(--border-default)]">
        <h2 className="text-xl sm:text-2xl font-bold tracking-tight text-[var(--fg-primary)]">
          Contact
        </h2>
      </div>

      <div className="space-y-4">
        <div className="contact-links flex flex-wrap items-center gap-x-7 gap-y-3 text-sm sm:text-base pt-2">
          <a
            href="mailto:contact@rahuldange.com"
            className="inline-flex items-center gap-2 py-2 text-[var(--fg-secondary)] hover:text-[var(--fg-primary)] transition-colors"
          >
            <IconMail size={15} className="text-[var(--accent-cyan)]" />
            <span className="truncate">contact@rahuldange.com</span>
          </a>

          <a
            href="https://github.com/rahuldangeofficial"
            target="_blank"
            rel="noopener noreferrer"
            className="inline-flex items-center gap-2 py-2 text-[var(--fg-secondary)] hover:text-[var(--fg-primary)] transition-colors"
          >
            <IconGithub size={15} className="text-[var(--accent-cyan)]" />
            <span>GitHub</span>
          </a>

          <a
            href="https://www.linkedin.com/in/rahuldangeofficial/"
            target="_blank"
            rel="noopener noreferrer"
            className="inline-flex items-center gap-2 py-2 text-[var(--fg-secondary)] hover:text-[var(--fg-primary)] transition-colors"
          >
            <IconLinkedin size={15} className="text-[var(--accent-cyan)]" />
            <span>LinkedIn</span>
          </a>
        </div>

        <div className="pt-3">
          <a
            href="/resume.pdf"
            download="Rahul Dange Resume.pdf"
            className="btn btn-primary btn-sm flex items-center gap-2  text-xs inline-flex"
          >
            <IconDownload size={14} />
            <span>Download Resume (PDF)</span>
          </a>
        </div>
      </div>

      {/* Minimal Footer */}
      <footer className="pt-12 pb-6 border-t border-[var(--border-subtle)] flex flex-col sm:flex-row items-start sm:items-center justify-between gap-2 text-xs  text-[var(--fg-muted)]">
        <div>© {new Date().getFullYear()} Rahul Dange</div>
        <div>rahuldange.com</div>
      </footer>
    </section>
  );
});

Contact.displayName = 'Contact';
export default Contact;
