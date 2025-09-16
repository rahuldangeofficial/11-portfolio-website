import React from 'react';
import { Card, Elevation, Icon, Tag, Button } from '@blueprintjs/core';
import { Calendar, Clock, ArrowRight } from 'lucide-react';

const BlogArticles: React.FC = () => {
  const articles = [
    {
      title: 'Optimizing React Performance: A Deep Dive into Rendering Patterns',
      excerpt: 'Exploring advanced techniques for optimizing React applications, including memoization strategies, virtual scrolling, and bundle splitting for better user experience.',
      date: '2024-01-15',
      readTime: '8 min read',
      category: 'Frontend',
      tags: ['React', 'Performance', 'JavaScript'],
      featured: true
    },
    {
      title: 'Building Scalable Microservices with Node.js and Docker',
      excerpt: 'A comprehensive guide to architecting microservices using Node.js, implementing proper service communication, and containerization best practices.',
      date: '2023-12-20',
      readTime: '12 min read',
      category: 'Backend',
      tags: ['Node.js', 'Docker', 'Microservices', 'Architecture']
    },
    {
      title: 'Memory Management in C++: Modern Techniques and Best Practices',
      excerpt: 'Understanding RAII, smart pointers, and memory optimization techniques in modern C++. Practical examples and performance comparisons included.',
      date: '2023-11-28',
      readTime: '15 min read',
      category: 'System Programming',
      tags: ['C++', 'Memory Management', 'Performance']
    },
    {
      title: 'Implementing Distributed Caching Strategies for High-Traffic Applications',
      excerpt: 'Exploring Redis clustering, cache invalidation patterns, and distributed caching architectures that can handle millions of requests per day.',
      date: '2023-10-10',
      readTime: '10 min read',
      category: 'System Design',
      tags: ['Redis', 'Caching', 'Distributed Systems']
    },
    {
      title: 'TypeScript Advanced Patterns: Generic Constraints and Conditional Types',
      excerpt: 'Deep dive into TypeScript\'s type system, exploring advanced patterns that can make your code more type-safe and maintainable.',
      date: '2023-09-15',
      readTime: '7 min read',
      category: 'Frontend',
      tags: ['TypeScript', 'Advanced Types', 'Type Safety']
    },
    {
      title: 'Database Indexing Strategies: PostgreSQL Performance Optimization',
      excerpt: 'Understanding different index types, query optimization techniques, and performance monitoring strategies for PostgreSQL databases.',
      date: '2023-08-22',
      readTime: '11 min read',
      category: 'Database',
      tags: ['PostgreSQL', 'Database', 'Performance', 'SQL']
    }
  ];

  const categories = ['All', 'Frontend', 'Backend', 'System Programming', 'System Design', 'Database'];

  const getCategoryColor = (category: string): 'primary' | 'success' | 'warning' | 'danger' | 'none' => {
    const colors: { [key: string]: 'primary' | 'success' | 'warning' | 'danger' | 'none' } = {
      'Frontend': 'primary',
      'Backend': 'success',
      'System Programming': 'warning',
      'System Design': 'danger',
      'Database': 'none'
    };
    return colors[category] || 'none';
  };

  return (
    <section id="blog" className="section-spacing bg-white">
      <div className="container mx-auto px-4 sm:px-6">
        <div className="max-w-6xl mx-auto">
          <h2 className="text-2xl sm:text-3xl font-bold text-center mb-6 md:mb-8 text-gray-800">Blog & Articles</h2>
          <p className="text-center text-gray-600 mb-8 md:mb-12 max-w-4xl mx-auto text-sm sm:text-base">
            Sharing insights, tutorials, and lessons learned from my experience in software engineering, 
            system programming, and web development. Each article aims to provide practical value to fellow developers.
          </p>
          
          {/* Category Filter */}
          <div className="flex flex-wrap justify-center gap-2 mb-8 md:mb-12">
            {categories.map((category, index) => (
              <Button
                key={index}
                minimal
                small
                className={`${index === 0 ? 'bg-blue-100 text-blue-700' : 'hover:bg-gray-100'} min-h-[36px] touch-manipulation text-xs sm:text-sm`}
              >
                {category}
              </Button>
            ))}
          </div>
          
          {/* Featured Article */}
          {articles.filter(article => article.featured).map((article, index) => (
            <Card 
              key={index}
              elevation={Elevation.TWO}
              className="profile-card p-4 sm:p-6 md:p-8 mb-6 md:mb-8 border-l-4 border-blue-500"
            >
              <div className="flex flex-wrap items-start gap-2 mb-3">
                <Tag intent="primary" className="flex-shrink-0 text-xs">Featured</Tag>
                <Tag 
                  intent={getCategoryColor(article.category)}
                  minimal
                  className="flex-shrink-0 text-xs"
                >
                  {article.category}
                </Tag>
              </div>
              
              <h3 className="text-lg sm:text-xl md:text-2xl font-bold text-gray-800 mb-4 hover:text-blue-600 cursor-pointer transition-colors">
                {article.title}
              </h3>
              
              <p className="text-gray-700 mb-4 leading-relaxed text-sm sm:text-base md:text-lg">
                {article.excerpt}
              </p>
              
              <div className="flex flex-col sm:flex-row sm:items-center sm:justify-between gap-4">
                <div className="flex flex-col sm:flex-row sm:items-center gap-2 sm:gap-4 text-gray-500 text-xs sm:text-sm">
                  <div className="flex items-center gap-1">
                    <Calendar size={12} className="sm:hidden" />
                    <Calendar size={14} className="hidden sm:block" />
                    <span>{new Date(article.date).toLocaleDateString('en-US', { 
                      year: 'numeric', 
                      month: 'long', 
                      day: 'numeric' 
                    })}</span>
                  </div>
                  <div className="flex items-center gap-1">
                    <Clock size={12} className="sm:hidden" />
                    <Clock size={14} className="hidden sm:block" />
                    <span>{article.readTime}</span>
                  </div>
                </div>
                
                <Button 
                  intent="primary"
                  small
                  className="sm:large min-h-[36px] sm:min-h-[44px] touch-manipulation"
                  rightIcon={<ArrowRight size={12} className="sm:hidden" />}
                >
                  <span className="hidden sm:inline">Read Article</span>
                  <span className="sm:hidden">Read</span>
                </Button>
              </div>
              
              <div className="flex flex-wrap gap-1.5 sm:gap-2 mt-4 pt-4 border-t border-gray-200">
                {article.tags.map((tag, tagIndex) => (
                  <Tag key={tagIndex} minimal className="skill-tag text-xs">
                    {tag}
                  </Tag>
                ))}
              </div>
            </Card>
          ))}
          
          {/* Regular Articles Grid */}
          <div className="grid grid-cols-1 lg:grid-cols-2 gap-4 sm:gap-6 mb-8 md:mb-12">
            {articles.filter(article => !article.featured).map((article, index) => (
              <Card 
                key={index}
                elevation={Elevation.ONE}
                className="profile-card p-4 sm:p-6 fade-in h-full flex flex-col"
              >
                <div className="flex items-start gap-2 mb-3">
                  <Tag 
                    intent={getCategoryColor(article.category)}
                    minimal
                    className="flex-shrink-0 text-xs"
                  >
                    {article.category}
                  </Tag>
                </div>
                
                <h3 className="text-base sm:text-lg md:text-xl font-semibold text-gray-800 mb-3 hover:text-blue-600 cursor-pointer transition-colors line-clamp-2">
                  {article.title}
                </h3>
                
                <p className="text-gray-600 mb-4 leading-relaxed text-xs sm:text-sm flex-grow">
                  {article.excerpt}
                </p>
                
                <div className="flex flex-col sm:flex-row sm:items-center sm:justify-between text-gray-500 text-xs mb-4 gap-2">
                  <div className="flex items-center gap-1">
                    <Calendar size={12} />
                    <span>{new Date(article.date).toLocaleDateString('en-US', { 
                      month: 'short', 
                      day: 'numeric',
                      year: 'numeric'
                    })}</span>
                  </div>
                  <div className="flex items-center gap-1">
                    <Clock size={12} />
                    <span>{article.readTime}</span>
                  </div>
                </div>
                
                <div className="flex flex-wrap gap-1 mb-4">
                  {article.tags.slice(0, 3).map((tag, tagIndex) => (
                    <Tag key={tagIndex} minimal className="text-xs">
                      {tag}
                    </Tag>
                  ))}
                  {article.tags.length > 3 && (
                    <Tag minimal className="text-xs">
                      +{article.tags.length - 3}
                    </Tag>
                  )}
                </div>
                
                <Button 
                  minimal
                  small
                  rightIcon={<ArrowRight size={12} />}
                  className="self-start hover:text-blue-600 transition-colors min-h-[36px] touch-manipulation"
                >
                  Read More
                </Button>
              </Card>
            ))}
          </div>

          {/* My Story Section */}
          <Card elevation={Elevation.ONE} className="profile-card p-4 sm:p-6 md:p-8 mb-8 md:mb-12">
            <div className="prose prose-sm sm:prose-base md:prose-lg max-w-none">
              <div className="flex flex-col sm:flex-row sm:items-center gap-3 mb-4 sm:mb-6">
                <div className="w-12 h-12 bg-blue-100 rounded-full flex items-center justify-center flex-shrink-0 mx-auto sm:mx-0">
                  <Icon icon="timeline-events" size={20} className="text-blue-600" />
                </div>
                <h3 className="text-xl sm:text-2xl font-semibold text-gray-800 m-0 text-center sm:text-left">My Story</h3>
              </div>
              
              <p className="text-gray-700 leading-relaxed mb-4 sm:mb-6 text-sm sm:text-base">
                My journey into software engineering began during my undergraduate years when I first encountered 
                the elegance of solving complex problems through code. What started as curiosity about how computers 
                work evolved into a deep passion for creating systems that make a real difference in people's lives.
              </p>
              
              <p className="text-gray-700 leading-relaxed mb-4 sm:mb-6 text-sm sm:text-base">
                The decision to pursue a Master's in Computer Science at the University of Sydney was driven by 
                my desire to understand not just how to write code, but how to architect robust, scalable systems. 
                During my studies, I dove deep into distributed systems, performance optimization, and software 
                engineering principles that continue to guide my work today.
              </p>
              
              <div className="bg-blue-50 rounded-lg p-4 sm:p-6 mb-4 sm:mb-6">
                <h4 className="text-base sm:text-lg font-semibold text-gray-800 mb-3 flex items-center gap-2">
                  <Icon icon="lightbulb" size={16} className="text-blue-600" />
                  The Turning Point
                </h4>
                <p className="text-gray-700 leading-relaxed text-sm sm:text-base">
                  A pivotal moment came during my thesis research on distributed caching systems. Working on 
                  performance optimization problems that affected thousands of users taught me that great 
                  software engineering isn't just about writing clean code—it's about understanding the entire 
                  ecosystem and making decisions that scale.
                </p>
              </div>
              
              <p className="text-gray-700 leading-relaxed mb-4 sm:mb-6 text-sm sm:text-base">
                Throughout my professional journey, from my early days as an intern to my current role as a 
                senior engineer, I've been fortunate to work on diverse projects—from high-performance system 
                programming to user-facing web applications. Each role has taught me something new about the 
                craft of software development and the importance of continuous learning.
              </p>
              
              <p className="text-gray-700 leading-relaxed text-sm sm:text-base">
                What drives me most is the intersection of technical excellence and real-world impact. Whether 
                I'm optimizing a database query that saves seconds for millions of users, mentoring a junior 
                developer who's just starting their journey, or contributing to open-source projects that help 
                the broader community, I find fulfillment in work that matters.
              </p>
            </div>
          </Card>
          
          {/* Newsletter Signup */}
          <Card elevation={Elevation.ONE} className="profile-card p-4 sm:p-6 md:p-8 text-center bg-blue-50">
            <div className="flex flex-col items-center">
              <div className="flex items-center justify-center gap-3 mb-4">
                <Icon icon="envelope" size={20} className="text-blue-600 sm:hidden" />
                <Icon icon="envelope" size={24} className="text-blue-600 hidden sm:block" />
                <h3 className="text-base sm:text-xl font-semibold text-gray-800 m-0">Stay Updated</h3>
              </div>
              <p className="text-gray-600 mb-4 sm:mb-6 max-w-2xl mx-auto text-xs sm:text-sm">
                Subscribe to get notified about new articles, tutorials, and insights on software engineering, 
                system programming, and web development. No spam, just valuable content.
              </p>
              <div className="flex flex-col sm:flex-row gap-2 w-full max-w-lg mx-auto">
                <input 
                  type="email" 
                  placeholder="Enter your email address"
                  className="flex-1 px-3 py-2 border border-gray-300 rounded-lg focus:outline-none focus:ring-2 focus:ring-blue-500 text-xs sm:text-sm h-9 sm:h-10"
                />
                <Button 
                  intent="primary" 
                  className="px-4 h-9 sm:h-10 touch-manipulation text-xs sm:text-sm"
                  small
                >
                  Subscribe
                </Button>
              </div>
            </div>
          </Card>
        </div>
      </div>
    </section>
  );
};

export default BlogArticles;