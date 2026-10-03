import React from 'react';
import { IconMail, IconPhone, IconGithub, IconLinkedin, IconDownload } from './Icons';
import { applicationSummary } from '../lib/profile';

const Contact: React.FC = React.memo(() => {
  return (
    <section id="contact" className="space-y-6 pt-4">
      <div className="pb-2 border-b border-[var(--border-default)]">
        <h2 className="text-xl sm:text-2xl font-bold tracking-tight text-[var(--fg-primary)]">
          Contact
        </h2>
      </div>

      <div className="space-y-4">
        <p className="text-sm sm:text-base text-[var(--fg-secondary)]">
          {applicationSummary}
        </p>

        <div className="grid grid-cols-1 sm:grid-cols-2 gap-3 text-sm sm:text-base  pt-2">
          <a
            href="mailto:contact@rahuldange.com"
            className="p-3 rounded border border-[var(--border-default)] hover:border-[var(--accent-cyan)] flex items-center gap-2.5 transition-colors"
          >
            <IconMail size={15} className="text-[var(--accent-cyan)]" />
            <span className="truncate">contact@rahuldange.com</span>
          </a>

          <a
            href="tel:+61412954045"
            className="p-3 rounded border border-[var(--border-default)] hover:border-[var(--accent-cyan)] flex items-center gap-2.5 transition-colors"
          >
            <IconPhone size={15} className="text-[var(--accent-cyan)]" />
            <span>+61 412 954 045</span>
          </a>

          <a
            href="https://github.com/rahuldangeofficial"
            target="_blank"
            rel="noopener noreferrer"
            className="p-3 rounded border border-[var(--border-default)] hover:border-[var(--accent-cyan)] flex items-center gap-2.5 transition-colors"
          >
            <IconGithub size={15} className="text-[var(--accent-cyan)]" />
            <span>github.com/rahuldangeofficial</span>
          </a>

          <a
            href="https://www.linkedin.com/in/rahuldangeofficial/"
            target="_blank"
            rel="noopener noreferrer"
            className="p-3 rounded border border-[var(--border-default)] hover:border-[var(--accent-cyan)] flex items-center gap-2.5 transition-colors"
          >
            <IconLinkedin size={15} className="text-[var(--accent-cyan)]" />
            <span>linkedin.com/in/rahuldangeofficial</span>
          </a>
        </div>

        <div className="pt-3">
          <a
            href="/resume.v4.pdf"
            download="resume.v4.pdf"
            className="btn btn-primary btn-sm flex items-center gap-2  text-xs inline-flex"
          >
            <IconDownload size={14} />
            <span>Download Resume (PDF)</span>
          </a>
        </div>
      </div>

      {/* Minimal Footer */}
      <footer className="pt-12 pb-6 border-t border-[var(--border-subtle)] flex flex-col sm:flex-row items-center justify-between gap-2 text-xs  text-[var(--fg-muted)]">
        <div>© {new Date().getFullYear()} Rahul Dange</div>
        <div>rahuldange.com</div>
      </footer>
    </section>
  );
});

Contact.displayName = 'Contact';
export default Contact;
