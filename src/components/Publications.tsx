import React from 'react';
import { IconExternalLink } from './Icons';

const Publications: React.FC = React.memo(() => {
  return (
    <section id="publications" className="space-y-6">
      <div className="pb-2 border-b border-[var(--border-default)]">
        <h2 className="text-xl sm:text-2xl font-bold tracking-tight text-[var(--fg-primary)]">
          Publications
        </h2>
      </div>

      <div className="space-y-6">
        <div className="space-y-1">
          <div className="flex items-start justify-between gap-2">
            <h3 className="text-base sm:text-lg font-bold text-[var(--fg-primary)]">
              Beyond the Representation
            </h3>
            <a
              href="https://medium.com/@rahuldangeofficial/beyond-the-representation-e0539a7335b7"
              target="_blank"
              rel="noopener noreferrer"
              className="text-xs text-[var(--accent-cyan)] hover:underline flex items-center gap-1 flex-shrink-0 pt-0.5"
            >
              <span>Medium</span>
              <IconExternalLink size={12} />
            </a>
          </div>
          <p className="text-xs text-[var(--fg-muted)]">
            Essay · <time dateTime="2026-08-26">26 August 2026</time> · 16 min read
          </p>
          <p className="text-sm sm:text-base text-[var(--fg-secondary)]">
            An exploratory essay on how the way we represent a problem shapes what we can solve. Connects mathematics, AI, and human cognition; explores systems that discover new abstractions; and proposes a representation compiler to translate unfamiliar structures into human-understandable concepts, with a prototype outline for testing these ideas.
          </p>
        </div>

        <div className="space-y-1">
          <div className="flex items-start justify-between gap-2">
            <h3 className="text-base sm:text-lg font-bold text-[var(--fg-primary)]">
              Revolutionizing Student Engagement: Building a Cutting-Edge Interaction System
            </h3>
            <a
              href="https://dx.doi.org/10.2139/ssrn.4625799"
              target="_blank"
              rel="noopener noreferrer"
              className="text-xs  text-[var(--accent-cyan)] hover:underline flex items-center gap-1 flex-shrink-0 pt-0.5"
            >
              <span>DOI Link</span>
              <IconExternalLink size={12} />
            </a>
          </div>
          <p className="text-sm sm:text-base text-[var(--fg-secondary)] ">
            SSRN (Elsevier), 5th International Conference on Communication &amp; Information Processing (ICCIP-2023).
          </p>
        </div>

        <div className="space-y-1 pt-2 border-t border-[var(--border-subtle)]">
          <h3 className="text-base sm:text-lg font-bold text-[var(--fg-primary)]">
            Textbook Code Correction: &quot;Let Us C&quot;
          </h3>
          <p className="text-sm sm:text-base text-[var(--fg-secondary)]">
            Discovered and reported a code implementation mistake in the 17th Edition of the C programming textbook <em>Let Us C</em> by Yashavant Kanetkar. Acknowledged by the author and scheduled for integration in the upcoming 18th Edition.
          </p>
        </div>
      </div>
    </section>
  );
});

Publications.displayName = 'Publications';
export default Publications;
