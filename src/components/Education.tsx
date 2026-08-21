import React from 'react';

const Education: React.FC = React.memo(() => {
  const educationList = [
    {
      institution: 'The University of Sydney',
      degree: 'Master of Computer Science (Software Engineering)',
      period: 'Feb 2024 - Dec 2025',
      location: 'Sydney, Australia'
    },
    {
      institution: 'Nutan College of Engineering and Research (NCER)',
      degree: 'Bachelor of Technology in Computer Science and Engineering',
      period: 'Aug 2019 - Aug 2023',
      location: 'Pune, India'
    }
  ];

  return (
    <section id="education" className="space-y-6">
      <div className="pb-2 border-b border-[var(--border-default)]">
        <h2 className="text-xl sm:text-2xl font-bold tracking-tight text-[var(--fg-primary)]">
          Education
        </h2>
      </div>

      <div className="space-y-6">
        {educationList.map((edu, idx) => (
          <div key={idx} className="space-y-1">
            <div className="flex flex-col sm:flex-row sm:items-baseline justify-between gap-1">
              <h3 className="text-base sm:text-lg font-bold text-[var(--fg-primary)]">
                {edu.institution}
              </h3>
              <span className="text-xs  text-[var(--fg-muted)]">
                {edu.period}
              </span>
            </div>

            <div className="flex flex-col sm:flex-row sm:items-baseline justify-between gap-1 text-xs sm:text-sm text-[var(--fg-secondary)]">
              <span className="font-medium">{edu.degree}</span>
              <span className="text-[var(--fg-muted)] ">{edu.location}</span>
            </div>
          </div>
        ))}
      </div>
    </section>
  );
});

Education.displayName = 'Education';
export default Education;