import React from 'react';

interface SkillCategory {
  title: string;
  icon: React.ReactNode;
  skills: string[];
}

const Skills: React.FC = React.memo(() => {
  const skillCategories: SkillCategory[] = React.useMemo(() => [
    {
      title: 'Systems & Low-Level',
      icon: (
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" className="w-5 h-5 text-[var(--accent-cyan)]">
          <rect x="4" y="4" width="16" height="16" rx="2" />
          <rect x="9" y="9" width="6" height="6" />
          <path d="M9 1v3M15 1v3M9 20v3M15 20v3M20 9h3M20 15h3M1 9h3M1 15h3" />
        </svg>
      ),
      skills: [
        'C',
        'C++',
        'x86-64 Assembly',
        'Linux Kernel',
        'POSIX Threads',
        'Concurrency',
        'IPC',
        'Memory Management',
        'GDB',
        'Valgrind'
      ]
    },
    {
      title: 'Languages & Paradigms',
      icon: (
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" className="w-5 h-5 text-[var(--accent-cyan)]">
          <polyline points="16 18 22 12 16 6" />
          <polyline points="8 6 2 12 8 18" />
        </svg>
      ),
      skills: [
        'Python',
        'Java',
        'TypeScript',
        'JavaScript',
        'SQL (PostgreSQL)',
        'Microservices Architecture',
        'System Design',
        'SOLID'
      ]
    },
    {
      title: 'Frameworks & APIs',
      icon: (
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" className="w-5 h-5 text-[var(--accent-cyan)]">
          <rect x="3" y="3" width="7" height="9" />
          <rect x="14" y="3" width="7" height="5" />
          <rect x="14" y="12" width="7" height="9" />
          <rect x="3" y="16" width="7" height="5" />
        </svg>
      ),
      skills: [
        'React',
        'Next.js',
        'Node.js',
        'Express',
        'FastAPI',
        'RESTful Architecture'
      ]
    },
    {
      title: 'Infrastructure & Telemetry',
      icon: (
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" className="w-5 h-5 text-[var(--accent-cyan)]">
          <path d="M18 10a5 5 0 0 0-9.5-2.5 4 4 0 0 0-4 4.5A4.5 4.5 0 0 0 9 21h9a4.5 4.5 0 0 0 0-9Z" />
        </svg>
      ),
      skills: [
        'Linux',
        'Bash/Shell',
        'Git',
        'Docker',
        'Google Cloud (GCP)',
        'AWS',
        'CI/CD (GitHub Actions)',
        'OpenTelemetry'
      ]
    }
  ], []);

  return (
    <section id="skills" className="section content-auto">
      <div className="container">
        <div className="max-w-4xl mx-auto">
          <h2 className="section-heading">Technical Skills</h2>
          <p className="section-subheading">
            A comprehensive look at the tools, languages, and architectures I leverage to build 
            robust, high-performance, and scalable systems.
          </p>

          <div className="grid grid-cols-1 md:grid-cols-2 gap-6 stagger-children">
            {skillCategories.map((category, index) => (
              <article 
                key={index} 
                className="card scroll-reveal p-4 sm:p-6 flex flex-col justify-between transition-all duration-300 hover:border-[var(--accent-cyan)] hover:shadow-glow-blue"
              >
                <div>
                  <div className="flex items-center gap-3 mb-5 pb-3 border-b border-[var(--border-subtle)]">
                    <div 
                      className="w-10 h-10 rounded-lg flex items-center justify-center bg-[var(--bg-surface)] border border-[var(--border-subtle)]"
                    >
                      {category.icon}
                    </div>
                    <h3
                      className="text-lg font-bold m-0 text-[var(--fg-primary)]"
                    >
                      {category.title}
                    </h3>
                  </div>
                  
                  <div className="flex flex-wrap gap-1.5">
                    {category.skills.map((skill) => (
                      <span key={skill} className="skill-tag">
                        {skill}
                      </span>
                    ))}
                  </div>
                </div>
              </article>
            ))}
          </div>
        </div>
      </div>
    </section>
  );
});

Skills.displayName = 'Skills';

export default Skills;