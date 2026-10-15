import React from 'react';

const roles = [
  {
    title: 'Student Advisor',
    dates: 'Jul 2022 - Jul 2023',
    description: 'Mentored junior students on software engineering careers and helped bring academic concerns to faculty leadership.',
  },
  {
    title: 'Primary Student Coordinator, NCER Hackathon',
    dates: '2022',
    description: 'Coordinated students across four CSE year groups and guided eight teams through technical challenges, product thinking and presentations.',
  },
  {
    title: 'NCER++ LMS Maintainer',
    dates: 'Feb 2020 - Sep 2023',
    description: 'Maintained a student resource hub with study materials, technical roadmaps and career guidance.',
  },
  {
    title: 'Founder & Coordinator, NCER Cyber Security Club',
    dates: 'Sep 2019 - Sep 2022',
    description: 'Started a cybersecurity learning community and managed its blog to share technical articles and learning resources.',
  },
  {
    title: 'Batch Representative, English Proficiency Initiative',
    dates: 'Aug 2019 - May 2020',
    description: 'Suggested teaching improvements for students from regional backgrounds and supported peers with English communication for professional preparation.',
  },
];

const Volunteering: React.FC = React.memo(() => (
  <section id="volunteering" className="space-y-6">
    <div className="pb-2 border-b border-[var(--border-default)]">
      <h2 className="text-xl sm:text-2xl font-bold tracking-tight text-[var(--fg-primary)]">Volunteering</h2>
    </div>
    <p className="text-xs sm:text-sm text-[var(--fg-muted)]">Nutan College of Engineering and Research, Pune</p>
    <div className="space-y-5">
      {roles.map(role => (
        <div key={role.title} className="space-y-1.5">
          <div className="flex flex-col sm:flex-row sm:items-baseline justify-between gap-1 sm:gap-4">
            <h3 className="text-sm sm:text-base font-semibold text-[var(--fg-primary)]">{role.title}</h3>
            <span className="text-xs text-[var(--fg-muted)] shrink-0">{role.dates}</span>
          </div>
          <p className="text-sm leading-relaxed text-[var(--fg-secondary)]">{role.description}</p>
        </div>
      ))}
    </div>
  </section>
));

Volunteering.displayName = 'Volunteering';
export default Volunteering;
