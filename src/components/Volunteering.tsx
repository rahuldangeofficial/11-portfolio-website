import React from 'react';
import { IconCalendar, IconMapPin, IconUsers } from './Icons';

interface VolunteeringItem {
  role: string;
  organization: string;
  period: string;
  location: string;
  description: string[];
}

const volunteeringList: VolunteeringItem[] = [
  {
    role: 'Senior Student Advisor',
    organization: 'Nutan College of Engineering & Research',
    period: 'Jul 2022 - Jul 2023 • 1 yr 1 mo',
    location: 'Pune, Maharashtra, India',
    description: [
      'I was appointed by the Head of the CS Department (Dr. Sanjeevkumar Angadi) to act as a strategic mentor for junior batches. My main focus was helping students transition from just "studying for a degree" to building a long-term career in software engineering.',
      'Beyond mentoring, I served as the primary link between the student body and faculty leadership, helping resolve academic roadblocks and making sure student concerns were heard and acted upon at the administrative level.'
    ]
  },
  {
    role: 'Primary Student Coordinator | NCER Hackathon 2022',
    organization: 'Nutan College of Engineering & Research',
    period: '2022 • Less than a year',
    location: 'Pune, Maharashtra, India',
    description: [
      'I took on the responsibility of being the sole primary student coordinator for the college\'s first-ever large-scale hackathon. It was a massive challenge as I had to represent and manage students across all four years of the CSE department.',
      'During the event, I worked closely with 8 different teams, guiding them through technical roadblocks, product thinking, and how to effectively present their ideas to judges. It was as much about project management as it was about technical mentorship.'
    ]
  },
  {
    role: 'NCER++ LMS Maintainer',
    organization: 'Nutan College of Engineering & Research',
    period: 'Feb 2020 - Sep 2023 • 3 yrs 8 mos',
    location: 'Pune, Maharashtra, India',
    description: [
      'This project started in 2019 as "NCER+" a simple Java application I built to help students manage academic resources. Over time, I saw a bigger need for career-specific guidance, so I evolved it into NCER++.',
      'The platform eventually became a hub for the student community, offering not just resources but also unit-specific technical roadmaps and career-building information to help students get industry-ready.'
    ]
  },
  {
    role: 'Founder & Coordinator | NCER Cyber Security Club',
    organization: 'Nutan College of Engineering & Research',
    period: 'Sep 2019 - Sep 2022 • 3 yrs 1 mo',
    location: 'Pune, Maharashtra, India',
    description: [
      'I started the first Cyber Security club in the history of NCER to build a community focused on ethical hacking and digital safety. It wasn\'t just about meetings, I also launched and managed the NCER Cybersecurity Blog site to share technical write-ups and security research. My goal was to create a platform where students could move beyond the curriculum and get hands-on with real-world security concepts.'
    ]
  },
  {
    role: 'Batch Representative | English Proficiency Initiative',
    organization: 'Nutan College of Engineering & Research',
    period: 'Aug 2019 - May 2020 • 10 mos',
    location: 'Pune, Maharashtra, India',
    description: [
      'This role started when I provided suggestions to improve the teaching methods of Mrs. Anuradha Sonawane, who was appointed by the college for this initiative. She was impressed by my ideas for making the program more effective for students from regional backgrounds, and subsequently declared me as the Batch Representative.',
      'I took that responsibility to help my peers bridge the communication gap, ensuring that everyone in our batch felt confident using English as a tool for their professional careers and placements.'
    ]
  }
];

const Volunteering: React.FC = React.memo(() => {
  return (
    <section id="volunteering" className="section content-auto">
      <div className="container">
        <h2 className="section-heading mb-8 md:mb-12">Volunteering &amp; Leadership</h2>
        
        <div className="max-w-4xl mx-auto space-y-6">
          {volunteeringList.map((item, idx) => (
            <div key={idx} className="card scroll-reveal p-4 sm:p-6 md:p-8 flex items-start gap-4 sm:gap-6 text-left">
              <div
                className="w-10 h-10 sm:w-12 sm:h-12 rounded-full flex items-center justify-center flex-shrink-0 bg-[var(--accent-cyan-glow-strong)]"
              >
                <span className="text-[var(--accent-cyan)]">
                  <IconUsers size={18} className="sm:hidden" />
                  <IconUsers size={20} className="hidden sm:block" />
                </span>
              </div>
              <div className="flex-1 space-y-3">
                <div>
                  <h3 className="text-lg sm:text-xl font-bold leading-snug text-[var(--fg-primary)]">
                    {item.role}
                  </h3>
                  <div className="flex flex-wrap items-center gap-x-3 gap-y-1.5 text-sm mt-1.5">
                    <span className="font-semibold text-[var(--accent-cyan)] w-full sm:w-auto">{item.organization}</span>
                    <span className="hide-on-mobile sm:inline text-[var(--border-default)]">•</span>
                    <span className="flex items-center gap-1.5 text-[var(--fg-muted)] w-full sm:w-auto">
                      <IconCalendar size={14} className="shrink-0" />
                      <span className="whitespace-nowrap">{item.period}</span>
                    </span>
                    <span className="hide-on-mobile sm:inline text-[var(--border-default)]">•</span>
                    <span className="flex items-center gap-1.5 text-[var(--fg-muted)] w-full sm:w-auto">
                      <IconMapPin size={14} className="shrink-0" />
                      <span className="whitespace-nowrap">{item.location}</span>
                    </span>
                  </div>
                </div>
                
                <div className="space-y-3">
                  {item.description.map((p, pIdx) => (
                    <p key={pIdx} className="leading-relaxed text-sm sm:text-base text-[var(--fg-secondary)]">
                      {p}
                    </p>
                  ))}
                </div>
              </div>
            </div>
          ))}
        </div>
      </div>
    </section>
  );
});

Volunteering.displayName = 'Volunteering';

export default Volunteering;
