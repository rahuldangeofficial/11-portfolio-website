import React, { memo } from 'react';
import { Card, Elevation, Tag } from '@blueprintjs/core';

const Skills: React.FC = memo(() => {
  const skillCategories = [
    {
      title: 'Programming Languages',
      skills: ['Python', 'JavaScript', 'TypeScript', 'Java', 'C++', 'C', 'Go', 'Rust', 'SQL']
    },
    {
      title: 'Web Technologies',
      skills: ['React', 'Node.js', 'Express.js', 'Next.js', 'Vue.js', 'HTML5', 'CSS3', 'REST APIs', 'GraphQL']
    },
    {
      title: 'System & DevOps',
      skills: ['Linux', 'Docker', 'Kubernetes', 'AWS', 'Azure', 'CI/CD', 'Jenkins', 'Git', 'Nginx']
    },
    {
      title: 'Databases',
      skills: ['PostgreSQL', 'MySQL', 'MongoDB', 'Redis', 'Elasticsearch', 'DynamoDB']
    },
    {
      title: 'Tools & Frameworks',
      skills: ['React Native', 'Django', 'Flask', 'Spring Boot', 'TensorFlow', 'PyTorch', 'Webpack', 'Vite']
    }
  ];

  return (
    <section id="skills" className="section-spacing bg-gray-50">
      <div className="container mx-auto px-4 sm:px-6">
        <div className="max-w-6xl mx-auto">
          <h2 className="text-2xl sm:text-3xl font-bold text-center mb-6 md:mb-8 text-gray-800">Technical Skills</h2>
          
          <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-4 sm:gap-6">
            {skillCategories.map((category, index) => (
              <Card 
                key={index}
                elevation={Elevation.ONE}
                className="profile-card p-4 sm:p-6 fade-in"
              >
                <h3 className="text-base sm:text-lg font-semibold mb-3 sm:mb-4 text-gray-800 pb-2 border-b border-gray-200">
                  {category.title}
                </h3>
                <div className="flex flex-wrap gap-1.5 sm:gap-2">
                  {category.skills.map((skill, skillIndex) => (
                    <Tag 
                      key={skillIndex}
                      className="skill-tag text-xs sm:text-sm"
                      minimal
                    >
                      {skill}
                    </Tag>
                  ))}
                </div>
              </Card>
            ))}
          </div>
        </div>
      </div>
    </section>
  );
});

Skills.displayName = 'Skills';

export default Skills;