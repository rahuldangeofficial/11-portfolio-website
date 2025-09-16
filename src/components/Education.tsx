import React from 'react';
import { Card, Elevation, Icon, Tag } from '@blueprintjs/core';

const Education: React.FC = () => {
  return (
    <section id="education" className="section-spacing bg-white">
      <div className="container mx-auto px-4 sm:px-6">
        <div className="max-w-4xl mx-auto">
          <h2 className="text-2xl sm:text-3xl font-bold text-center mb-6 md:mb-8 text-gray-800">Education</h2>
          
          <Card elevation={Elevation.ONE} className="profile-card p-4 sm:p-6 md:p-8 fade-in">
            <div className="flex flex-col xl:flex-row xl:items-start gap-6 md:gap-8">
              <div className="flex-1">
                <div className="flex flex-col sm:flex-row sm:items-center gap-3 mb-4">
                  <div className="w-12 h-12 bg-blue-100 rounded-full flex items-center justify-center flex-shrink-0">
                    <Icon icon="learning" className="text-blue-600" size={20} />
                  </div>
                  <div className="text-center sm:text-left">
                    <h3 className="text-xl sm:text-2xl font-semibold text-gray-800">
                      Master of Computer Science
                    </h3>
                    <p className="text-base sm:text-lg font-medium text-blue-600">University of Sydney</p>
                  </div>
                </div>
                
                <div className="flex flex-col sm:flex-row sm:items-center gap-2 sm:gap-4 text-gray-600 mb-4 text-sm sm:text-base">
                  <div className="flex items-center justify-center sm:justify-start gap-1">
                    <Icon icon="calendar" size={14} />
                    <span className="font-medium">2017 - 2019</span>
                  </div>
                  <div className="flex items-center justify-center sm:justify-start gap-1">
                    <Icon icon="map-marker" size={14} />
                    <span>Sydney, Australia</span>
                  </div>
                </div>
                
                <div className="flex justify-center sm:justify-start mb-6">
                  <Tag intent="success" large className="text-center">
                    <Icon icon="star" className="mr-2" size={14} />
                    Specialization: Software Engineering
                  </Tag>
                </div>
                
                <div className="bg-blue-50 rounded-lg p-4 sm:p-6 mb-6">
                  <h4 className="text-base sm:text-lg font-semibold text-gray-800 mb-3">Master's Thesis</h4>
                  <h5 className="text-base sm:text-lg font-medium text-blue-700 mb-2">
                    "Optimizing Distributed Systems Performance through Advanced Caching Strategies"
                  </h5>
                  <p className="text-gray-700 leading-relaxed text-sm sm:text-base">
                    Research focused on improving system performance in large-scale distributed environments. 
                    Developed novel caching algorithms that achieved 35% improvement in response times for 
                    microservices architectures. Work was published in the International Conference on 
                    Distributed Computing Systems.
                  </p>
                </div>
                
                <div className="mb-6">
                  <h4 className="text-base sm:text-lg font-semibold text-gray-800 mb-3">Academic Achievements</h4>
                  <ul className="space-y-2">
                    <li className="flex items-center gap-2">
                      <Icon icon="star" className="text-yellow-500 flex-shrink-0" size={16} />
                      <span className="text-gray-700 text-sm sm:text-base">Dean's List - High Distinction Average</span>
                    </li>
                    <li className="flex items-center gap-2">
                      <Icon icon="star" className="text-yellow-500 flex-shrink-0" size={16} />
                      <span className="text-gray-700 text-sm sm:text-base">Research Excellence Award 2019</span>
                    </li>
                    <li className="flex items-center gap-2">
                      <Icon icon="star" className="text-yellow-500 flex-shrink-0" size={16} />
                      <span className="text-gray-700 text-sm sm:text-base">Published Research Paper - ICDCS 2019</span>
                    </li>
                  </ul>
                </div>
              </div>
              
              <div className="xl:w-80">
                <div className="bg-gray-50 rounded-lg p-4 sm:p-6">
                  <h4 className="text-base sm:text-lg font-semibold text-gray-800 mb-4">Core Coursework</h4>
                  <div className="space-y-3">
                    {[
                      'Advanced Software Engineering',
                      'Distributed Systems Architecture',
                      'Database Systems & Design',
                      'Algorithms & Data Structures',
                      'Computer Networks & Security',
                      'Machine Learning & AI',
                      'Software Testing & Quality Assurance',
                      'Agile Project Management',
                      'Human-Computer Interaction',
                      'Cloud Computing & DevOps'
                    ].map((course, index) => (
                      <div key={index} className="flex items-start gap-2">
                        <Icon icon="tick" className="text-green-600 flex-shrink-0 mt-0.5" size={14} />
                        <span className="text-gray-700 text-xs sm:text-sm">{course}</span>
                      </div>
                    ))}
                  </div>
                </div>
              </div>
            </div>
          </Card>
        </div>
      </div>
    </section>
  );
};

export default Education;