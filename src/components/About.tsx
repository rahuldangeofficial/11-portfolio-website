import React, { memo } from 'react';
import { Card, Elevation } from '@blueprintjs/core';

const About: React.FC = memo(() => {
  return (
    <section id="about" className="section-spacing bg-white">
      <div className="container mx-auto px-4 sm:px-6">
        <div className="max-w-4xl mx-auto">
          <h2 className="text-2xl sm:text-3xl font-bold text-center mb-6 md:mb-8 text-gray-800">About Me</h2>
          
          <Card elevation={Elevation.ONE} className="profile-card p-4 sm:p-6 md:p-8 fade-in">
            <div className="prose prose-sm sm:prose-base md:prose-lg max-w-none">
              <p className="text-gray-700 leading-relaxed mb-4 text-sm sm:text-base">
                I am a passionate and dedicated software engineer with a Master's degree in Computer Science 
                from the University of Sydney, specializing in software engineering. With extensive experience 
                in system programming, software development, and web application development, I bring a unique 
                combination of theoretical knowledge and practical expertise to every project.
              </p>
              
              <p className="text-gray-700 leading-relaxed mb-4 text-sm sm:text-base">
                My journey in technology has been driven by a commitment to creating robust, scalable, and 
                efficient solutions. I excel in designing and implementing complex systems, from low-level 
                system programming to high-level web applications, always with a focus on best practices, 
                performance optimization, and maintainable code architecture.
              </p>
              
              <p className="text-gray-700 leading-relaxed text-sm sm:text-base">
                I thrive in collaborative environments where I can contribute to innovative projects, mentor 
                junior developers, and continuously expand my technical expertise. My goal is to leverage 
                cutting-edge technologies to solve real-world problems and create meaningful impact through 
                software engineering excellence.
              </p>
            </div>
          </Card>
        </div>
      </div>
    </section>
  );
});

About.displayName = 'About';

export default About;