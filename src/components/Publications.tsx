import React from 'react';
import {
  IconBook,
  IconExternalLink,
} from './Icons';

interface Publication {
  title: string;
  citation: string;
  link: string;
  description: string;
  date: string;
}

const publicationsList: Publication[] = [
  {
    title: 'Revolutionizing Student Engagement: Building a Cutting-Edge Student Interaction System with MERN Stack and Next.js',
    citation: 'SSRN (Elsevier) / 5th International Conference on Communication and Information Processing (ICCIP-2023) • Nov 7, 2023',
    link: 'https://dx.doi.org/10.2139/ssrn.4625799',
    date: '2023-11-07',
    description: 'This research was born out of my work on the NCER++ project. I wanted to explore how modern architectural patterns like MVC and server-side rendering (SSR) could solve performance lags in traditional education systems. In this paper, I detailed the implementation of a high-performance interaction platform using the MERN stack and Next.js. I focused on evaluating system response times under heavy concurrent loads and how technologies like TypeScript, Cloudinary, and optimized asset loading (Turbopack) contribute to a seamless user experience. It essentially serves as the technical blueprint for the platforms I\'ve built to improve student-educator engagement.'
  },
  {
    title: 'Student Interaction System Using Modern Web Technologies',
    citation: 'Scandinavian Journal of Information Systems, 35(1), 142-151, 2023. • Feb 6, 2023',
    link: 'https://doi.org/10.5281/zenodo.7858182',
    date: '2023-02-06',
    description: 'Modern online technologies have the ability to significantly increase the efficacy and efficiency of systems for student engagement. In this study, we examine the research on web based student engagement systems that support and improve the learning process. We look at the many student interactions that are facilitated by these platforms, including online conversations, group writing, and conferencing, and we evaluate how they affect student development and learning outcomes. We also talk about the potential and problems that come with using web technologies in student engagement systems, such as accessibility and scalability concerns. Finally, we offer suggestions for teachers and designers who want to use web technologies to create efficient student engagement systems that enhance learning and student growth.'
  }
];

const Publications: React.FC = React.memo(() => {
  return (
    <section id="publications" className="section section-alt content-auto">
      <div className="container">
        <div className="max-w-4xl mx-auto">
          <h2 className="section-heading">Publications &amp; Contributions</h2>
          <p className="section-subheading">
            Sharing research, academic publications, and contributions to the software engineering community.
          </p>

          {/* Academic Publications */}
          <div className="space-y-6 md:space-y-8 mb-8 md:mb-12">
            <h3 className="text-xl sm:text-2xl font-bold border-b border-[var(--border-subtle)] pb-2 mb-4 text-left text-[var(--fg-primary)]">
              Peer-Reviewed Publications
            </h3>
            {publicationsList.map((pub, index) => (
              <article 
                key={index} 
                className="card scroll-reveal p-4 sm:p-6 md:p-8 flex flex-col justify-between"
              >
                <div>
                  <h4 
                    className="text-lg sm:text-xl font-bold mb-3 text-left text-[var(--fg-primary)]"
                  >
                    {pub.title}
                  </h4>
                  
                  <p className="text-xs sm:text-sm font-semibold mb-4 text-left text-[var(--accent-cyan)]">
                    {pub.citation}
                  </p>
                  
                  <p className="text-sm leading-relaxed mb-6 text-left text-[var(--fg-secondary)]">
                    {pub.description}
                  </p>
                </div>
                
                <div className="flex justify-start border-t border-[var(--border-subtle)] pt-4">
                  <a 
                    href={pub.link}
                    target="_blank"
                    rel="noopener noreferrer"
                    className="btn btn-secondary btn-sm flex items-center gap-1.5"
                  >
                    Show Publication
                    <IconExternalLink size={14} />
                  </a>
                </div>
              </article>
            ))}
          </div>

          {/* Contributions Section */}
          <div className="mb-8 md:mb-12">
            <h3 className="text-xl sm:text-2xl font-bold border-b border-[var(--border-subtle)] pb-2 mb-4 text-left text-[var(--fg-primary)]">
              Community &amp; Academic Contributions
            </h3>
            
            <article className="card scroll-reveal p-4 sm:p-6 md:p-8 flex items-start gap-4 sm:gap-6 text-left">
              <div 
                className="w-10 h-10 sm:w-12 sm:h-12 rounded-full flex items-center justify-center flex-shrink-0 bg-[var(--accent-cyan-glow)] border border-[var(--accent-cyan)]"
              >
                <IconBook size={18} className="text-[var(--accent-cyan)] sm:hidden" />
                <IconBook size={20} className="text-[var(--accent-cyan)] hidden sm:block" />
              </div>
              <div className="flex-1 space-y-2">
                <h4 className="text-lg sm:text-xl font-bold leading-snug text-[var(--fg-primary)]">
                  Textbook Code Correction &quot;Let Us C &quot; Book
                </h4>
                <p className="text-xs sm:text-sm font-semibold text-[var(--accent-cyan)]">
                  Reporting &amp; Collaborating with Yashavant Kanetkar
                </p>
                <p className="text-sm leading-relaxed text-[var(--fg-secondary)]">
                  Discovered and reported a code implementation mistake in the 17th Edition of the widely acclaimed C programming textbook <strong>Let Us C</strong> by Yashavant Kanetkar. After communicating the details, the author acknowledged the correction and confirmed that the fix will be integrated into the upcoming 18th Edition of the book.
                </p>
              </div>
            </article>
          </div>
        </div>
      </div>
    </section>
  );
});

Publications.displayName = 'Publications';

export default Publications;
