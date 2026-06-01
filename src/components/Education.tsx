import React from 'react';
import { IconGraduationCap, IconCalendar, IconMapPin } from './Icons';

interface EducationItem {
  degree: string;
  institution: string;
  period: string;
  location: string;
}

const educationList: EducationItem[] = [
  {
    degree: 'Master of Computer Science (Software Engineering Specialization)',
    institution: 'University of Sydney',
    period: '2024 Feb - 2025 Nov',
    location: 'Sydney, NSW, Australia'
  },
  {
    degree: 'Bachelor of Technology (B.Tech) in Computer Science & Engineering',
    institution: 'Nutan College of Engineering & Research (NCER)',
    period: '2019 July - 2023 July',
    location: 'Pune, Maharashtra, India'
  }
];

const Education: React.FC = React.memo(() => {
  return (
    <section id="education" className="section content-auto">
      <div className="container">
        <h2 className="section-heading mb-8 md:mb-12">Education</h2>
        
        <div className="max-w-4xl mx-auto space-y-6">
          {educationList.map((edu, idx) => (
            <div key={idx} className="card scroll-reveal p-4 sm:p-6 md:p-8 flex items-start gap-4 sm:gap-6 text-left">
              <div
                className="w-10 h-10 sm:w-12 sm:h-12 rounded-full flex items-center justify-center flex-shrink-0 bg-[var(--accent-cyan-glow-strong)]"
              >
                <span className="text-[var(--accent-cyan)]">
                  <IconGraduationCap size={18} className="sm:hidden" />
                  <IconGraduationCap size={20} className="hidden sm:block" />
                </span>
              </div>
              <div className="flex-1 space-y-1">
                <h3 className="text-lg sm:text-xl font-bold leading-snug text-[var(--fg-primary)]">
                  {edu.degree}
                </h3>
                <p className="text-sm sm:text-base font-semibold text-[var(--accent-cyan)]">
                  {edu.institution}
                </p>
                <div className="flex flex-wrap gap-x-4 gap-y-1 text-xs sm:text-sm font-medium pt-1 text-[var(--fg-muted)]">
                  <div className="flex items-center gap-1.5">
                    <IconCalendar size={14} />
                    <span>{edu.period}</span>
                  </div>
                  <div className="flex items-center gap-1.5">
                    <IconMapPin size={14} />
                    <span>{edu.location}</span>
                  </div>
                </div>
              </div>
            </div>
          ))}
        </div>
      </div>
    </section>
  );
});

Education.displayName = 'Education';

export default Education;