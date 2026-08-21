import React from 'react';

const Skills: React.FC = React.memo(() => {
  const skillCategories = [
    {
      title: 'Programming',
      skills: 'C, C++, Java, Python, TypeScript, JavaScript, SQL, x86-64 Assembly'
    },
    {
      title: 'Systems',
      skills: 'C11 atomics, POSIX threads, memory management, system calls, signals, IPC, Linux'
    },
    {
      title: 'Debugging & Testing',
      skills: 'GDB, Valgrind, AddressSanitizer (ASan), ThreadSanitizer (TSan), UndefinedBehaviorSanitizer (UBSan)'
    },
    {
      title: 'Backend & Web',
      skills: 'Java Spring Boot, Node.js, Express.js, REST APIs, PostgreSQL, MongoDB, React.js, HTML/CSS'
    },
    {
      title: 'Tools',
      skills: 'Git, Docker, AWS, Make, Bash'
    }
  ];

  return (
    <section id="skills" className="space-y-6">
      <div className="pb-2 border-b border-[var(--border-default)]">
        <h2 className="text-xl sm:text-2xl font-bold tracking-tight text-[var(--fg-primary)]">
          Technical Skills
        </h2>
      </div>

      <div className="space-y-3">
        {skillCategories.map((cat, idx) => (
          <div key={idx} className="flex flex-col sm:flex-row sm:items-baseline gap-1 sm:gap-4 text-sm sm:text-base">
            <span className="font-bold text-[var(--fg-primary)] sm:w-44 flex-shrink-0">
              {cat.title}:
            </span>
            <span className="text-[var(--fg-secondary)] text-sm sm:text-base">
              {cat.skills}
            </span>
          </div>
        ))}
      </div>
    </section>
  );
});

Skills.displayName = 'Skills';
export default Skills;