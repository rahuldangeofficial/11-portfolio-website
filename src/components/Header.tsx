import React, { useState, useEffect, useMemo } from 'react';
import { Button, Icon } from '@blueprintjs/core';
import { Mail, Phone, MapPin, Github, Linkedin, Download, Menu, X } from 'lucide-react';

const Header: React.FC = () => {
  const [isMenuOpen, setIsMenuOpen] = useState(false);
  const [activeSection, setActiveSection] = useState('about');

  const navigationItems = useMemo(() => [
    { id: 'about', label: 'About' },
    { id: 'skills', label: 'Skills' },
    { id: 'projects', label: 'Projects' },
    { id: 'education', label: 'Education' },
    { id: 'preferences', label: 'Tech Stack' },
    { id: 'interests', label: 'Interests' },
    { id: 'blog', label: 'Blog' },
    { id: 'contact', label: 'Contact' }
  ], []);

  useEffect(() => {
    const handleScroll = () => {
      const sections = navigationItems.map(item => document.getElementById(item.id));
      const scrollPosition = window.scrollY + 100;

      for (let i = sections.length - 1; i >= 0; i--) {
        const section = sections[i];
        if (section && section.offsetTop <= scrollPosition) {
          setActiveSection(navigationItems[i].id);
          break;
        }
      }
    };

    window.addEventListener('scroll', handleScroll);
    return () => window.removeEventListener('scroll', handleScroll);
  }, [navigationItems]);

  const scrollToSection = (sectionId: string) => {
    const element = document.getElementById(sectionId);
    if (element) {
      const headerOffset = 80;
      const elementPosition = element.offsetTop;
      const offsetPosition = elementPosition - headerOffset;

      window.scrollTo({
        top: offsetPosition,
        behavior: 'smooth'
      });
    }
    setIsMenuOpen(false);
  };

  return (
    <>
      {/* Main Header */}
      <div className="bg-white border-b border-gray-200 py-8 md:py-12">
        <div className="container mx-auto px-4 sm:px-6">
          <div className="max-w-4xl mx-auto">
            <div className="text-center">
              {/* Profile Image */}
              <div className="w-24 h-24 sm:w-28 sm:h-28 md:w-32 md:h-32 bg-gray-100 rounded-full mx-auto mb-4 md:mb-6 flex items-center justify-center border-4 border-white shadow-lg">
                <Icon icon="person" size={32} className="text-gray-400 sm:hidden" />
                <Icon icon="person" size={40} className="text-gray-400 hidden sm:block md:hidden" />
                <Icon icon="person" size={48} className="text-gray-400 hidden md:block" />
              </div>
              
              {/* Name and Title */}
              <h1 className="text-2xl sm:text-3xl md:text-4xl font-bold mb-2 md:mb-3 text-gray-900 px-2">Rahul Dange</h1>
              <p className="text-base sm:text-lg md:text-xl mb-2 text-gray-600 px-2">
                System Programmer • Software Engineer • Web Application Developer
              </p>
              <p className="text-sm sm:text-base md:text-lg mb-6 md:mb-8 text-gray-500 px-2">
                Master of Computer Science (Software Engineering) • University of Sydney
              </p>
              
              {/* Contact Information */}
              <div className="flex flex-col sm:flex-row sm:flex-wrap justify-center gap-3 sm:gap-4 md:gap-6 mb-6 md:mb-8 text-gray-600 px-2">
                <div className="flex items-center justify-center gap-2 text-sm sm:text-base">
                  <Mail size={14} className="sm:hidden" />
                  <Mail size={16} className="hidden sm:block" />
                  <span className="break-all sm:break-normal">rahul.dange@email.com</span>
                </div>
                <div className="flex items-center justify-center gap-2 text-sm sm:text-base">
                  <Phone size={14} className="sm:hidden" />
                  <Phone size={16} className="hidden sm:block" />
                  <span>+61 XXX XXX XXX</span>
                </div>
                <div className="flex items-center justify-center gap-2 text-sm sm:text-base">
                  <MapPin size={14} className="sm:hidden" />
                  <MapPin size={16} className="hidden sm:block" />
                  <span>Sydney, Australia</span>
                </div>
              </div>
              
              {/* Action Buttons */}
              <div className="flex flex-col sm:flex-row justify-center gap-3 sm:gap-4 px-4">
                <Button 
                  intent="primary"
                  large
                  className="w-full sm:w-auto min-h-[44px] touch-manipulation"
                  icon={<Download size={16} />}
                >
                  Download Resume
                </Button>
                <Button 
                  outlined
                  large
                  className="w-full sm:w-auto min-h-[44px] touch-manipulation"
                  icon={<Github size={16} />}
                >
                  GitHub
                </Button>
                <Button 
                  outlined
                  large
                  className="w-full sm:w-auto min-h-[44px] touch-manipulation"
                  icon={<Linkedin size={16} />}
                >
                  LinkedIn
                </Button>
              </div>
            </div>
          </div>
        </div>
      </div>

      {/* Sticky Navigation */}
      <nav className="sticky top-0 z-50 bg-white/95 backdrop-blur-sm border-b border-gray-200 shadow-sm">
        <div className="container mx-auto px-4 sm:px-6">
          <div className="max-w-6xl mx-auto">
            {/* Desktop Navigation */}
            <div className="hidden lg:flex items-center justify-center py-4">
              <div className="flex items-center space-x-1">
                {navigationItems.map((item) => (
                  <button
                    key={item.id}
                    onClick={() => scrollToSection(item.id)}
                    className={`px-4 py-2 rounded-lg text-sm font-medium transition-all duration-200 hover:bg-gray-100 ${
                      activeSection === item.id
                        ? 'bg-blue-100 text-blue-700'
                        : 'text-gray-600 hover:text-gray-900'
                    }`}
                  >
                    {item.label}
                  </button>
                ))}
              </div>
            </div>

            {/* Mobile Navigation */}
            <div className="lg:hidden flex items-center justify-between py-3">
              <div className="flex items-center space-x-2">
                <div className="w-8 h-8 bg-blue-100 rounded-full flex items-center justify-center">
                  <Icon icon="person" size={16} className="text-blue-600" />
                </div>
                <span className="font-semibold text-gray-900">Rahul Dange</span>
              </div>
              
              <button
                onClick={() => setIsMenuOpen(!isMenuOpen)}
                className="p-2 rounded-lg hover:bg-gray-100 transition-colors"
              >
                {isMenuOpen ? <X size={20} /> : <Menu size={20} />}
              </button>
            </div>

            {/* Mobile Menu */}
            {isMenuOpen && (
              <div className="lg:hidden absolute top-full left-0 right-0 bg-white border-b border-gray-200 shadow-lg">
                <div className="px-4 py-2 space-y-1">
                  {navigationItems.map((item) => (
                    <button
                      key={item.id}
                      onClick={() => scrollToSection(item.id)}
                      className={`w-full text-left px-4 py-3 rounded-lg text-sm font-medium transition-all duration-200 ${
                        activeSection === item.id
                          ? 'bg-blue-100 text-blue-700'
                          : 'text-gray-600 hover:bg-gray-50 hover:text-gray-900'
                      }`}
                    >
                      {item.label}
                    </button>
                  ))}
                </div>
              </div>
            )}
          </div>
        </div>
      </nav>
    </>
  );
};

export default Header;