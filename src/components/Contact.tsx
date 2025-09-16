import React from 'react';
import { Card, Elevation, Button } from '@blueprintjs/core';
import { Mail, Phone, MapPin, Github, Linkedin, Globe, MessageCircle } from 'lucide-react';

const Contact: React.FC = () => {
  return (
    <section id="contact" className="section-spacing bg-gray-50">
      <div className="container mx-auto px-4 sm:px-6">
        <div className="max-w-4xl mx-auto">
          <h2 className="text-2xl sm:text-3xl font-bold text-center mb-6 md:mb-8 text-gray-800">Let's Connect</h2>
          
          <div className="grid grid-cols-1 lg:grid-cols-2 gap-6 md:gap-8 mb-8 md:mb-12">
            <Card elevation={Elevation.ONE} className="profile-card p-4 sm:p-6">
              <h3 className="text-lg sm:text-xl font-semibold mb-4 sm:mb-6 text-gray-800 flex items-center gap-2">
                <MessageCircle size={20} className="text-gray-600" />
                Get In Touch
              </h3>
              <div className="space-y-4">
                <div className="flex items-start gap-3 sm:gap-4">
                  <div className="w-10 h-10 sm:w-12 sm:h-12 bg-gray-100 rounded-full flex items-center justify-center flex-shrink-0">
                    <Mail size={16} className="text-gray-600 sm:hidden" />
                    <Mail size={18} className="text-gray-600 hidden sm:block" />
                  </div>
                  <div className="min-w-0 flex-1">
                    <p className="text-gray-600 text-xs sm:text-sm">Email</p>
                    <p className="text-gray-800 font-medium text-sm sm:text-base break-all">rahul.dange@email.com</p>
                    <p className="text-gray-500 text-xs">Best for detailed discussions</p>
                  </div>
                </div>
                
                <div className="flex items-start gap-3 sm:gap-4">
                  <div className="w-10 h-10 sm:w-12 sm:h-12 bg-gray-100 rounded-full flex items-center justify-center flex-shrink-0">
                    <Phone size={16} className="text-gray-600 sm:hidden" />
                    <Phone size={18} className="text-gray-600 hidden sm:block" />
                  </div>
                  <div>
                    <p className="text-gray-600 text-xs sm:text-sm">Phone</p>
                    <p className="text-gray-800 font-medium text-sm sm:text-base">+61 XXX XXX XXX</p>
                    <p className="text-gray-500 text-xs">Available 9 AM - 6 PM AEST</p>
                  </div>
                </div>
                
                <div className="flex items-start gap-3 sm:gap-4">
                  <div className="w-10 h-10 sm:w-12 sm:h-12 bg-gray-100 rounded-full flex items-center justify-center flex-shrink-0">
                    <MapPin size={16} className="text-gray-600 sm:hidden" />
                    <MapPin size={18} className="text-gray-600 hidden sm:block" />
                  </div>
                  <div>
                    <p className="text-gray-600 text-xs sm:text-sm">Location</p>
                    <p className="text-gray-800 font-medium text-sm sm:text-base">Sydney, Australia</p>
                    <p className="text-gray-500 text-xs">Open to remote opportunities</p>
                  </div>
                </div>
              </div>
            </Card>
            
            <Card elevation={Elevation.ONE} className="profile-card p-4 sm:p-6">
              <h3 className="text-lg sm:text-xl font-semibold mb-4 sm:mb-6 text-gray-800">Professional Links</h3>
              <div className="space-y-3">
                <Button
                  large
                  fill
                  minimal
                  className="justify-start hover:bg-gray-50 min-h-[44px] touch-manipulation"
                  icon={<Github size={18} />}
                >
                  <div className="text-left">
                    <div className="font-medium text-sm sm:text-base">GitHub</div>
                    <div className="text-xs sm:text-sm text-gray-500">View my code repositories</div>
                  </div>
                </Button>
                
                <Button
                  large
                  fill
                  minimal
                  className="justify-start hover:bg-gray-50 min-h-[44px] touch-manipulation"
                  icon={<Linkedin size={18} />}
                >
                  <div className="text-left">
                    <div className="font-medium text-sm sm:text-base">LinkedIn</div>
                    <div className="text-xs sm:text-sm text-gray-500">Professional network & experience</div>
                  </div>
                </Button>
                
                <Button
                  large
                  fill
                  minimal
                  className="justify-start hover:bg-gray-50 min-h-[44px] touch-manipulation"
                  icon={<Globe size={18} />}
                >
                  <div className="text-left">
                    <div className="font-medium text-sm sm:text-base">Portfolio Website</div>
                    <div className="text-xs sm:text-sm text-gray-500">Detailed project showcase</div>
                  </div>
                </Button>
              </div>
              
              <div className="mt-4 sm:mt-6 pt-4 border-t border-gray-200">
                <h4 className="font-semibold text-gray-800 mb-2 text-sm sm:text-base">Response Time</h4>
                <p className="text-gray-600 text-xs sm:text-sm leading-relaxed">
                  I typically respond to emails within 24 hours. For urgent matters, 
                  feel free to reach out via phone or LinkedIn for faster communication.
                </p>
              </div>
            </Card>
          </div>
          
          <div className="text-center mt-8 md:mt-12 pt-6 md:pt-8 border-t border-gray-200">
            <p className="text-gray-500 mb-2 text-sm sm:text-base">
              © 2024 Rahul Dange. All rights reserved.
            </p>
            <p className="text-gray-400 text-xs sm:text-sm">
              Built with React, TypeScript, and Blueprint.js
            </p>
          </div>
        </div>
      </div>
    </section>
  );
};

export default Contact;