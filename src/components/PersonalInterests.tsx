import React from 'react';
import { Card, Elevation, Icon } from '@blueprintjs/core';

const PersonalInterests: React.FC = () => {
  const interests = [
    {
      icon: 'code',
      title: 'Open Source Contributions',
      description: 'Active contributor to various open-source projects, particularly in system programming and web development tools. Maintaining several npm packages with 10K+ downloads.',
      activities: ['Contributing to React ecosystem', 'Maintaining CLI tools', 'Code reviews and mentoring']
    },
    {
      icon: 'learning',
      title: 'Continuous Learning',
      description: 'Passionate about staying current with emerging technologies and programming paradigms. Regular participant in tech conferences and online courses.',
      activities: ['Rust programming language', 'Machine Learning applications', 'Cloud architecture patterns']
    },
    {
      icon: 'camera',
      title: 'Photography & Travel',
      description: 'Landscape and street photography enthusiast. Love exploring new places and capturing moments that tell stories. This hobby enhances my attention to detail and creative problem-solving.',
      activities: ['Digital photography', 'Photo editing and processing', 'Travel blogging']
    },
    {
      icon: 'heart',
      title: 'Fitness & Wellness',
      description: 'Regular gym-goer and hiking enthusiast. Believe in maintaining work-life balance through physical activities. Often find that physical challenges help with mental clarity in coding.',
      activities: ['Weight training', 'Hiking and bushwalking', 'Meditation and mindfulness']
    },
    {
      icon: 'book',
      title: 'Technical Reading',
      description: 'Avid reader of technical books, research papers, and industry publications. Always exploring new concepts in computer science and software engineering.',
      activities: ['System design books', 'Research papers', 'Tech industry blogs']
    },
    {
      icon: 'people',
      title: 'Community Involvement',
      description: 'Active member of local tech meetups and programming communities. Enjoy sharing knowledge through talks and workshops, and learning from fellow developers.',
      activities: ['Sydney Tech Meetups', 'Coding workshops', 'Mentoring students']
    }
  ];

  return (
    <section id="interests" className="section-spacing bg-white">
      <div className="container mx-auto px-6">
        <div className="max-w-6xl mx-auto">
          <h2 className="text-3xl font-bold text-center mb-8 text-gray-800">Personal Interests & Hobbies</h2>
          <p className="text-center text-gray-600 mb-12 max-w-3xl mx-auto">
            Beyond coding, I'm passionate about various activities that contribute to my growth as both a developer and an individual. 
            These interests often inspire creative solutions in my professional work.
          </p>
          
          <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
            {interests.map((interest, index) => (
              <Card 
                key={index}
                elevation={Elevation.ONE}
                className="profile-card p-6 fade-in h-full"
              >
                <div className="text-center mb-4">
                  <div className="w-16 h-16 bg-blue-100 rounded-full mx-auto mb-4 flex items-center justify-center">
                    <Icon icon={interest.icon as 'code' | 'learning' | 'camera' | 'heart'} size={24} className="text-blue-600" />
                  </div>
                  <h3 className="text-xl font-semibold text-gray-800 mb-3">
                    {interest.title}
                  </h3>
                </div>
                
                <p className="text-gray-700 mb-4 leading-relaxed text-sm">
                  {interest.description}
                </p>
                
                <div>
                  <h4 className="font-semibold text-gray-800 mb-2 text-sm">Current Focus:</h4>
                  <ul className="space-y-1">
                    {interest.activities.map((activity, actIndex) => (
                      <li key={actIndex} className="flex items-center gap-2">
                        <Icon icon="dot" className="text-blue-600 flex-shrink-0" size={12} />
                        <span className="text-gray-600 text-sm">{activity}</span>
                      </li>
                    ))}
                  </ul>
                </div>
              </Card>
            ))}
          </div>
        </div>
      </div>
    </section>
  );
};

export default PersonalInterests;