import React from 'react';
import { IconLightbulb, IconLayout, IconDatabase, IconCpu, IconCloud } from './Icons';

interface PreferenceItem {
  title: string;
  description: string;
  preference: 'Essential' | 'Preferred' | 'Flexible' | 'Context-dependent';
}

interface PreferenceCategory {
  category: string;
  icon: React.FC<{ size?: number; className?: string }>;
  items: PreferenceItem[];
}

const preferences: PreferenceCategory[] = [
  {
    category: 'Development Philosophy',
    icon: IconLightbulb,
    items: [
      {
        title: 'Clean Code Principles',
        description: 'Write code that is readable, maintainable, and self-documenting. Follow SOLID principles and design patterns.',
        preference: 'Essential'
      },
      {
        title: 'Test-Driven Development',
        description: 'Write tests first to ensure code quality and reduce bugs. Aim for high test coverage with meaningful tests.',
        preference: 'Preferred'
      },
      {
        title: 'Agile Methodologies',
        description: 'Iterative development with regular feedback loops. Scrum and Kanban for project management.',
        preference: 'Preferred'
      }
    ]
  },
  {
    category: 'Frontend Development',
    icon: IconLayout,
    items: [
      {
        title: 'React Ecosystem',
        description: 'React with TypeScript for type safety. Next.js for full-stack applications. Tailwind CSS for styling.',
        preference: 'Preferred'
      },
      {
        title: 'State Management',
        description: 'Context API for simple state, Redux Toolkit for complex applications. Zustand for lightweight solutions.',
        preference: 'Flexible'
      },
      {
        title: 'Build Tools',
        description: 'Vite for fast development, Webpack for complex configurations. ESLint and Prettier for code quality.',
        preference: 'Essential'
      }
    ]
  },
  {
    category: 'Backend Development',
    icon: IconDatabase,
    items: [
      {
        title: 'Node.js & Express',
        description: 'Express.js for REST APIs, Fastify for high-performance applications. TypeScript for type safety.',
        preference: 'Preferred'
      },
      {
        title: 'Database Choice',
        description: 'PostgreSQL for relational data, MongoDB for document storage, Redis for caching and sessions.',
        preference: 'Context-dependent'
      },
      {
        title: 'API Design',
        description: 'RESTful APIs with proper HTTP status codes. GraphQL for complex data requirements. OpenAPI documentation.',
        preference: 'Essential'
      }
    ]
  },
  {
    category: 'System Programming',
    icon: IconCpu,
    items: [
      {
        title: 'Language Choice',
        description: 'C++ for performance-critical applications, Rust for memory safety, Go for concurrent systems.',
        preference: 'Context-dependent'
      },
      {
        title: 'Concurrency Models',
        description: 'Async/await patterns, thread pools for CPU-bound tasks, event-driven architecture for I/O.',
        preference: 'Essential'
      },
      {
        title: 'Performance Optimization',
        description: 'Profiling-driven optimization, memory management, algorithm efficiency, caching strategies.',
        preference: 'Essential'
      }
    ]
  },
  {
    category: 'DevOps & Infrastructure',
    icon: IconCloud,
    items: [
      {
        title: 'Containerization',
        description: 'Docker for development and production, Kubernetes for orchestration, Docker Compose for local development.',
        preference: 'Preferred'
      },
      {
        title: 'CI/CD Pipelines',
        description: 'GitHub Actions for automation, Jenkins for enterprise, automated testing and deployment.',
        preference: 'Essential'
      },
      {
        title: 'Cloud Platforms',
        description: 'AWS for comprehensive services, Azure for enterprise integration, serverless for event-driven applications.',
        preference: 'Flexible'
      }
    ]
  }
];

const getPreferenceTagClass = (preference: string): string => {
  switch (preference) {
    case 'Essential': return 'tag tag-green';
    case 'Preferred': return 'tag tag-blue';
    case 'Flexible': return 'tag';
    case 'Context-dependent': return 'tag';
    default: return 'tag';
  }
};

const frameworkItems = [
  { label: 'Essential', tagClass: 'tag tag-green', description: 'Non-negotiable for quality and maintainability' },
  { label: 'Preferred', tagClass: 'tag tag-blue', description: 'Go-to choice based on experience and results' },
  { label: 'Flexible', tagClass: 'tag', description: 'Open to alternatives based on team/project needs' },
  { label: 'Context-dependent', tagClass: 'tag', description: 'Choice depends on specific requirements' }
];

const TechnicalPreferences: React.FC = React.memo(() => {
  return (
    <section id="preferences" className="section section-alt content-auto">
      <div className="container">
        <div className="max-w-4xl mx-auto">
          <h2 className="section-heading">Technical Stack Preferences</h2>
          <p className="section-subheading">
            My approach to technology selection is based on project requirements, team expertise, and long-term maintainability.
            Here are my preferences and the reasoning behind them.
          </p>

          <div className="space-y-8">
            {preferences.map((category, categoryIndex) => {
              const CategoryIcon = category.icon;
              return (
                <div key={categoryIndex} className="card scroll-reveal p-4 sm:p-6 md:p-8">
                  <div className="flex items-center gap-3 mb-6">
                    <div
                      className="w-10 h-10 rounded-lg flex items-center justify-center bg-[var(--accent-cyan-glow-strong)]"
                    >
                      <span className="text-[var(--accent-cyan)]"><CategoryIcon size={20} /></span>
                    </div>
                    <h3 className="text-xl md:text-2xl font-semibold text-[var(--fg-primary)]">
                      {category.category}
                    </h3>
                  </div>

                  <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
                    {category.items.map((item, itemIndex) => (
                      <div
                        key={itemIndex}
                        className="rounded-lg p-4 bg-[var(--bg-surface)] border border-[var(--border-subtle)]"
                      >
                        <div className="flex items-start justify-between mb-3">
                          <h4 className="font-semibold text-lg text-[var(--fg-primary)]">
                            {item.title}
                          </h4>
                          <span className={`${getPreferenceTagClass(item.preference)} ml-2 flex-shrink-0`}>
                            {item.preference}
                          </span>
                        </div>
                        <p className="text-sm leading-relaxed text-[var(--fg-muted)]">
                          {item.description}
                        </p>
                      </div>
                    ))}
                  </div>
                </div>
              );
            })}
          </div>

          {/* Decision Framework */}
          <div className="card p-4 sm:p-6 mt-8">
            <h3 className="text-xl font-semibold mb-4 text-center text-[var(--fg-primary)]">
              Decision Framework
            </h3>
            <div className="grid grid-cols-1 md:grid-cols-4 gap-4 text-center">
              {frameworkItems.map((item, index) => (
                <div key={index} id={`framework-${item.label.toLowerCase().replace(/\s+/g, '-')}`}>
                  <span className={`${item.tagClass} mb-2 inline-block`}>{item.label}</span>
                  <p className="text-sm text-[var(--fg-muted)]">{item.description}</p>
                </div>
              ))}
            </div>
          </div>
        </div>
      </div>
    </section>
  );
});

export default TechnicalPreferences;