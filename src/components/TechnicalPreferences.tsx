import React from 'react';
import { Card, Elevation, Icon, Tag } from '@blueprintjs/core';

const TechnicalPreferences: React.FC = () => {
  const preferences = [
    {
      category: 'Development Philosophy',
      icon: 'lightbulb',
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
      icon: 'application',
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
      icon: 'database',
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
      icon: 'cog',
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
      icon: 'cloud',
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

  const getPreferenceColor = (preference: string): 'primary' | 'success' | 'warning' | 'danger' | 'none' => {
    switch (preference) {
      case 'Essential': return 'success';
      case 'Preferred': return 'primary';
      case 'Flexible': return 'warning';
      case 'Context-dependent': return 'none';
      default: return 'none';
    }
  };

  return (
    <section id="preferences" className="section-spacing bg-gray-50">
      <div className="container mx-auto px-6">
        <div className="max-w-6xl mx-auto">
          <h2 className="text-3xl font-bold text-center mb-8 text-gray-800">Technical Stack Preferences</h2>
          <p className="text-center text-gray-600 mb-12 max-w-4xl mx-auto">
            My approach to technology selection is based on project requirements, team expertise, and long-term maintainability. 
            Here are my preferences and the reasoning behind them.
          </p>
          
          <div className="space-y-8">
            {preferences.map((category, categoryIndex) => (
              <Card 
                key={categoryIndex}
                elevation={Elevation.ONE}
                className="profile-card p-8 fade-in"
              >
                <div className="flex items-center gap-3 mb-6">
                  <div className="w-12 h-12 bg-blue-100 rounded-full flex items-center justify-center">
                    <Icon icon={category.icon as 'desktop' | 'database' | 'cloud' | 'cog'} size={20} className="text-blue-600" />
                  </div>
                  <h3 className="text-2xl font-semibold text-gray-800">{category.category}</h3>
                </div>
                
                <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
                  {category.items.map((item, itemIndex) => (
                    <div key={itemIndex} className="bg-white rounded-lg p-4 border border-gray-200">
                      <div className="flex items-start justify-between mb-3">
                        <h4 className="font-semibold text-gray-800 text-lg">{item.title}</h4>
                        <Tag 
                          intent={getPreferenceColor(item.preference)}
                          minimal
                          className="ml-2 flex-shrink-0"
                        >
                          {item.preference}
                        </Tag>
                      </div>
                      <p className="text-gray-600 text-sm leading-relaxed">{item.description}</p>
                    </div>
                  ))}
                </div>
              </Card>
            ))}
          </div>
          
          <Card elevation={Elevation.ONE} className="profile-card p-6 mt-8">
            <h3 className="text-xl font-semibold text-gray-800 mb-4 text-center">Decision Framework</h3>
            <div className="grid grid-cols-1 md:grid-cols-4 gap-4 text-center">
              <div>
                <Tag intent="success" large className="mb-2">Essential</Tag>
                <p className="text-sm text-gray-600">Non-negotiable for quality and maintainability</p>
              </div>
              <div>
                <Tag intent="primary" large className="mb-2">Preferred</Tag>
                <p className="text-sm text-gray-600">Go-to choice based on experience and results</p>
              </div>
              <div>
                <Tag intent="warning" large className="mb-2">Flexible</Tag>
                <p className="text-sm text-gray-600">Open to alternatives based on team/project needs</p>
              </div>
              <div>
                <Tag large className="mb-2">Context-dependent</Tag>
                <p className="text-sm text-gray-600">Choice depends on specific requirements</p>
              </div>
            </div>
          </Card>
        </div>
      </div>
    </section>
  );
};

export default TechnicalPreferences;