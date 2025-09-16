import React from 'react';
import { Card, Elevation, Tag, Icon } from '@blueprintjs/core';
import { Github, ExternalLink, Star } from 'lucide-react';

const Projects: React.FC = () => {
  const projects = [
    // Featured Projects (4)
    {
      title: 'DistributedCache Pro',
      category: 'System Programming',
      status: 'Production',
      description: 'High-performance distributed caching system built in Rust with custom memory management. Implements consistent hashing, automatic failover, and real-time replication across multiple nodes. Achieves sub-millisecond response times with 99.99% uptime.',
      highlights: [
        'Handles 1M+ requests per second with <0.5ms latency',
        'Custom memory allocator reduces GC overhead by 90%',
        'Automatic cluster rebalancing and fault tolerance',
        'RESTful API with WebSocket support for real-time updates'
      ],
      technologies: ['Rust', 'Redis Protocol', 'Docker', 'Kubernetes', 'Prometheus', 'gRPC'],
      github: 'https://github.com/rahuldange/distributed-cache-pro',
      demo: 'https://cache-demo.rahuldange.dev',
      stars: 847,
      featured: true
    },
    {
      title: 'Real-time Collaboration Platform',
      category: 'Web Development',
      status: 'Production',
      description: 'Full-stack web application for real-time collaborative document editing with operational transforms, WebRTC video calls, and advanced permission management. Built with modern web technologies and microservices architecture.',
      highlights: [
        'Supports 10,000+ concurrent users per document',
        'Real-time collaborative editing with conflict resolution',
        'WebRTC integration for video calls and screen sharing',
        'Advanced role-based access control and audit logging'
      ],
      technologies: ['React', 'Node.js', 'TypeScript', 'WebRTC', 'Socket.io', 'MongoDB', 'Redis'],
      github: 'https://github.com/rahuldange/collab-platform',
      demo: 'https://collab.rahuldange.dev',
      stars: 1156,
      featured: true
    },
    {
      title: 'Neural Network Compiler',
      category: 'System Programming',
      status: 'Open Source',
      description: 'LLVM-based compiler that optimizes neural network computations for various hardware targets. Implements graph optimization, automatic differentiation, and hardware-specific code generation for CPUs, GPUs, and custom accelerators.',
      highlights: [
        'Cross-platform compilation for x86, ARM, and CUDA',
        'Graph-level optimizations reduce inference time by 60%',
        'Automatic memory layout optimization',
        'Support for dynamic shapes and control flow'
      ],
      technologies: ['C++', 'LLVM', 'CUDA', 'Python', 'CMake', 'MLIR', 'TensorFlow'],
      github: 'https://github.com/rahuldange/nn-compiler',
      demo: 'https://nn-compiler.rahuldange.dev',
      stars: 1243,
      featured: true
    },
    {
      title: 'E-Commerce Analytics Dashboard',
      category: 'Web Development',
      status: 'Production',
      description: 'Comprehensive web-based analytics platform for e-commerce businesses with real-time data visualization, predictive analytics, and automated reporting. Features interactive dashboards, custom metrics, and integration with major e-commerce platforms.',
      highlights: [
        'Processes 100M+ transactions daily with real-time insights',
        'ML-powered sales forecasting with 95% accuracy',
        'Interactive dashboards with 50+ customizable widgets',
        'Integration with Shopify, WooCommerce, and Magento'
      ],
      technologies: ['Vue.js', 'Python', 'FastAPI', 'PostgreSQL', 'Redis', 'D3.js', 'TensorFlow'],
      github: 'https://github.com/rahuldange/ecommerce-analytics',
      demo: 'https://analytics-demo.rahuldange.dev',
      stars: 689,
      featured: true
    },
    
    // Regular Projects (6)
    {
      title: 'Container Orchestration Engine',
      category: 'System Programming',
      status: 'Production',
      description: 'Lightweight container orchestration platform optimized for edge computing. Features intelligent scheduling, resource optimization, and seamless failover. Designed for IoT and edge deployment scenarios.',
      highlights: [
        'Manages 10,000+ containers across edge nodes',
        'Intelligent scheduling based on latency and resource constraints',
        'Automatic failover with <5 second recovery time',
        'Optimized for ARM and low-power devices'
      ],
      technologies: ['Go', 'containerd', 'etcd', 'gRPC', 'Prometheus', 'ARM'],
      github: 'https://github.com/rahuldange/edge-orchestrator',
      demo: 'https://edge-orch.rahuldange.dev',
      stars: 667,
      featured: false
    },
    {
      title: 'Social Media Management Suite',
      category: 'Web Development',
      status: 'Production',
      description: 'Comprehensive web application for managing multiple social media accounts with content scheduling, analytics, and team collaboration. Features AI-powered content suggestions and automated posting across platforms.',
      highlights: [
        'Manages 50+ social media platforms simultaneously',
        'AI-powered content optimization and scheduling',
        'Advanced analytics with engagement prediction',
        'Team collaboration with approval workflows'
      ],
      technologies: ['React', 'Node.js', 'GraphQL', 'PostgreSQL', 'Redis', 'AWS', 'OpenAI API'],
      github: 'https://github.com/rahuldange/social-suite',
      demo: 'https://social.rahuldange.dev',
      stars: 543,
      featured: false
    },
    {
      title: 'Time Series Database',
      category: 'System Programming',
      status: 'Production',
      description: 'High-performance time series database with compression, aggregation, and real-time querying. Optimized for IoT data ingestion and monitoring workloads. Handles billions of data points with microsecond query latency.',
      highlights: [
        'Ingests 1M+ data points per second per node',
        'Custom compression achieving 20:1 ratios for time series data',
        'Real-time aggregation with sliding window operations',
        'Distributed architecture with automatic sharding'
      ],
      technologies: ['Rust', 'RocksDB', 'Apache Arrow', 'gRPC', 'Kubernetes', 'Prometheus'],
      github: 'https://github.com/rahuldange/tsdb-engine',
      demo: 'https://tsdb.rahuldange.dev',
      stars: 834,
      featured: false
    },
    {
      title: 'Project Management Platform',
      category: 'Web Development',
      status: 'Production',
      description: 'Modern web-based project management platform with Kanban boards, Gantt charts, time tracking, and team collaboration. Features real-time updates, custom workflows, and integration with popular development tools.',
      highlights: [
        'Supports teams of 1,000+ members with real-time sync',
        'Custom workflow automation with 100+ integrations',
        'Advanced reporting with burndown and velocity charts',
        'Mobile-responsive design with offline capabilities'
      ],
      technologies: ['Angular', 'NestJS', 'TypeScript', 'PostgreSQL', 'Socket.io', 'Docker'],
      github: 'https://github.com/rahuldange/project-mgmt',
      demo: 'https://pm.rahuldange.dev',
      stars: 721,
      featured: false
    },
    {
      title: 'WebAssembly Runtime',
      category: 'System Programming',
      status: 'Open Source',
      description: 'High-performance WebAssembly runtime with JIT compilation and sandboxing. Implements WASI support, memory management optimizations, and cross-platform execution. Designed for serverless and edge computing environments.',
      highlights: [
        'JIT compilation with 5x faster execution than interpreters',
        'Memory-safe sandboxing with capability-based security',
        'WASI support for system calls and file operations',
        'Hot-swappable modules for zero-downtime updates'
      ],
      technologies: ['Rust', 'LLVM', 'WebAssembly', 'WASI', 'Cranelift', 'Wasmtime'],
      github: 'https://github.com/rahuldange/wasm-runtime',
      demo: 'https://wasm-runtime.rahuldange.dev',
      stars: 623,
      featured: false
    },
    {
      title: 'Learning Management System',
      category: 'Web Development',
      status: 'Production',
      description: 'Comprehensive web-based LMS with course creation, video streaming, interactive assessments, and progress tracking. Features adaptive learning algorithms, plagiarism detection, and multi-language support.',
      highlights: [
        'Serves 100,000+ students across 50+ institutions',
        'Adaptive learning paths with personalized recommendations',
        'HD video streaming with adaptive bitrate',
        'Advanced plagiarism detection with 98% accuracy'
      ],
      technologies: ['React', 'Django', 'Python', 'PostgreSQL', 'Redis', 'FFmpeg', 'AWS'],
      github: 'https://github.com/rahuldange/lms-platform',
      demo: 'https://lms.rahuldange.dev',
      stars: 456,
      featured: false
    },

    // Academic/Research Projects (4)
    {
      title: 'Distributed Consensus in Byzantine Networks',
      category: 'Academic Research',
      status: 'Research',
      description: 'Master\'s thesis research on novel consensus algorithms for Byzantine fault-tolerant distributed systems. Developed and formally verified a new consensus protocol with improved performance characteristics under adversarial conditions.',
      highlights: [
        'Formal verification using TLA+ and model checking',
        'Improved throughput by 40% over existing protocols',
        'Published in IEEE Distributed Systems Conference',
        'Implemented and tested on 100-node testbed'
      ],
      technologies: ['TLA+', 'Go', 'Docker', 'Kubernetes', 'Prometheus', 'LaTeX'],
      github: 'https://github.com/rahuldange/byzantine-consensus',
      demo: 'https://consensus-research.rahuldange.dev',
      stars: 234,
      featured: false,
      academic: true
    },
    {
      title: 'Machine Learning for Code Optimization',
      category: 'Academic Research',
      status: 'Research',
      description: 'Research project applying reinforcement learning to compiler optimization passes. Developed ML models that automatically select optimal compiler flags and transformation sequences for improved performance.',
      highlights: [
        'Achieved 25% average performance improvement over -O3',
        'Trained on 10,000+ C/C++ programs from open source',
        'Published in ACM PLDI conference proceedings',
        'Integration with LLVM compiler infrastructure'
      ],
      technologies: ['Python', 'TensorFlow', 'LLVM', 'C++', 'Reinforcement Learning', 'CUDA'],
      github: 'https://github.com/rahuldange/ml-compiler-opt',
      demo: 'https://ml-opt.rahuldange.dev',
      stars: 312,
      featured: false,
      academic: true
    },
    {
      title: 'Quantum Error Correction Simulator',
      category: 'Academic Research',
      status: 'Research',
      description: 'Undergraduate research project developing a quantum error correction simulator for surface codes. Implemented various decoding algorithms and analyzed their performance under different noise models.',
      highlights: [
        'Simulates surface codes up to 1000 qubits',
        'Implemented 5 different decoding algorithms',
        'Comprehensive noise model analysis',
        'Contributed to 2 peer-reviewed publications'
      ],
      technologies: ['Python', 'NumPy', 'Qiskit', 'C++', 'OpenMP', 'Matplotlib'],
      github: 'https://github.com/rahuldange/quantum-ecc',
      demo: 'https://quantum-ecc.rahuldange.dev',
      stars: 189,
      featured: false,
      academic: true
    },
    {
      title: 'Parallel Graph Algorithms on GPU',
      category: 'Academic Research',
      status: 'Research',
      description: 'Course project implementing parallel graph algorithms on GPU using CUDA. Developed optimized implementations of BFS, DFS, shortest path, and community detection algorithms for large-scale graphs.',
      highlights: [
        'Achieved 100x speedup over CPU implementations',
        'Handles graphs with 100M+ vertices and 1B+ edges',
        'Memory-efficient algorithms for GPU constraints',
        'Comprehensive performance analysis and benchmarking'
      ],
      technologies: ['CUDA', 'C++', 'Python', 'NetworkX', 'Thrust', 'CUB'],
      github: 'https://github.com/rahuldange/gpu-graph-algorithms',
      demo: 'https://gpu-graphs.rahuldange.dev',
      stars: 167,
      featured: false,
      academic: true
    }
  ];

  const getStatusColor = (status: string): 'primary' | 'success' | 'warning' | 'danger' | 'none' => {
    switch (status) {
      case 'Production': return 'success';
      case 'Open Source': return 'primary';
      case 'Research': return 'warning';
      default: return 'none';
    }
  };

  const getCategoryColor = (category: string): 'primary' | 'success' | 'warning' | 'danger' | 'none' => {
    switch (category) {
      case 'System Programming': return 'danger';
      case 'Web Development': return 'primary';
      case 'Academic Research': return 'warning';
      case 'Machine Learning Systems': return 'primary';
      case 'Database Systems': return 'success';
      case 'Distributed Systems': return 'warning';
      case 'Cloud Infrastructure': return 'primary';
      case 'Big Data': return 'success';
      default: return 'none';
    }
  };

  return (
    <section id="projects" className="section-spacing bg-gray-50">
      <div className="container mx-auto px-4 sm:px-6">
        <div className="max-w-6xl mx-auto">
          <h2 className="text-2xl sm:text-3xl font-bold text-center mb-6 md:mb-8 text-gray-800">Featured Projects</h2>
          
          {/* Featured Projects */}
          <div className="mb-8 md:mb-12">
            <h3 className="text-xl font-semibold text-gray-800 mb-6">Highlighted Work</h3>
            <div className="grid grid-cols-1 lg:grid-cols-2 gap-6 md:gap-8">
              {projects.filter(project => project.featured).map((project, index) => (
                <Card 
                  key={index}
                  elevation={Elevation.TWO}
                  className="profile-card p-6 md:p-8 fade-in border-l-4 border-blue-500"
                >
                  <div className="flex items-start justify-between mb-4">
                    <div className="flex-1">
                      <div className="flex items-center gap-2 mb-2">
                        <Tag intent={getCategoryColor(project.category)} minimal className="text-xs">
                          {project.category}
                        </Tag>
                        <Tag intent={getStatusColor(project.status)} className="text-xs">
                          {project.status}
                        </Tag>
                      </div>
                      <h3 className="text-xl md:text-2xl font-bold text-gray-800 mb-2">{project.title}</h3>
                    </div>
                    <div className="flex items-center gap-1 text-gray-500">
                      <Star size={16} className="fill-yellow-400 text-yellow-400" />
                      <span className="text-sm font-medium">{project.stars}</span>
                    </div>
                  </div>
                  
                  <p className="text-gray-700 mb-4 leading-relaxed text-sm md:text-base">{project.description}</p>
                  
                  <div className="mb-4">
                    <h4 className="text-base font-semibold text-gray-800 mb-3">Key Highlights</h4>
                    <ul className="space-y-2">
                      {project.highlights.map((highlight, hIndex) => (
                        <li key={hIndex} className="flex items-start gap-3">
                          <Icon icon="tick-circle" className="text-green-600 mt-1 flex-shrink-0" size={14} />
                          <span className="text-gray-700 text-sm">{highlight}</span>
                        </li>
                      ))}
                    </ul>
                  </div>
                  
                  <div className="mb-4">
                    <h4 className="text-base font-semibold text-gray-800 mb-3">Technologies</h4>
                    <div className="flex flex-wrap gap-1.5">
                      {project.technologies.map((tech, techIndex) => (
                        <Tag key={techIndex} className="skill-tag text-xs" minimal>
                          {tech}
                        </Tag>
                      ))}
                    </div>
                  </div>
                  
                  <div className="flex gap-3 pt-4 border-t border-gray-200">
                    <a 
                      href={project.github}
                      className="flex items-center gap-2 px-4 py-2 bg-gray-800 text-white rounded-lg hover:bg-gray-700 transition-colors text-sm"
                    >
                      <Github size={16} />
                      Code
                    </a>
                    <a 
                      href={project.demo}
                      className="flex items-center gap-2 px-4 py-2 bg-blue-600 text-white rounded-lg hover:bg-blue-700 transition-colors text-sm"
                    >
                      <ExternalLink size={16} />
                      Demo
                    </a>
                  </div>
                </Card>
              ))}
            </div>
          </div>

          {/* Regular Projects */}
          <div className="mb-8 md:mb-12">
            <h3 className="text-xl font-semibold text-gray-800 mb-6">Additional Projects</h3>
            <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
              {projects.filter(project => !project.featured && !project.academic).map((project, index) => (
                <Card 
                  key={index}
                  elevation={Elevation.ONE}
                  className="profile-card p-6 fade-in h-full flex flex-col"
                >
                  <div className="flex items-start justify-between mb-3">
                    <div className="flex-1">
                      <div className="flex items-center gap-2 mb-2">
                        <Tag intent={getCategoryColor(project.category)} minimal className="text-xs">
                          {project.category}
                        </Tag>
                        <Tag intent={getStatusColor(project.status)} className="text-xs">
                          {project.status}
                        </Tag>
                      </div>
                      <h3 className="text-lg font-bold text-gray-800 mb-2">{project.title}</h3>
                    </div>
                    <div className="flex items-center gap-1 text-gray-500">
                      <Star size={14} className="fill-yellow-400 text-yellow-400" />
                      <span className="text-xs font-medium">{project.stars}</span>
                    </div>
                  </div>
                  
                  <p className="text-gray-700 mb-4 leading-relaxed text-sm flex-1">{project.description}</p>
                  
                  <div className="mb-4">
                    <div className="flex flex-wrap gap-1.5">
                      {project.technologies.slice(0, 4).map((tech, techIndex) => (
                        <Tag key={techIndex} className="skill-tag text-xs" minimal>
                          {tech}
                        </Tag>
                      ))}
                      {project.technologies.length > 4 && (
                        <Tag className="skill-tag text-xs" minimal>
                          +{project.technologies.length - 4} more
                        </Tag>
                      )}
                    </div>
                  </div>
                  
                  <div className="flex gap-2 pt-3 border-t border-gray-200">
                    <a 
                      href={project.github}
                      className="flex items-center gap-2 px-3 py-1.5 bg-gray-800 text-white rounded text-xs hover:bg-gray-700 transition-colors"
                    >
                      <Github size={14} />
                      Code
                    </a>
                    <a 
                      href={project.demo}
                      className="flex items-center gap-2 px-3 py-1.5 bg-blue-600 text-white rounded text-xs hover:bg-blue-700 transition-colors"
                    >
                      <ExternalLink size={14} />
                      Demo
                    </a>
                  </div>
                </Card>
              ))}
            </div>
          </div>

          {/* Academic/Research Projects */}
          <div>
            <h3 className="text-xl font-semibold text-gray-800 mb-6">Academic & Research Projects</h3>
            <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
              {projects.filter(project => project.academic).map((project, index) => (
                <Card 
                  key={index}
                  elevation={Elevation.ONE}
                  className="profile-card p-6 fade-in h-full flex flex-col border-l-4 border-orange-400"
                >
                  <div className="flex items-start justify-between mb-3">
                    <div className="flex-1">
                      <div className="flex items-center gap-2 mb-2">
                        <Tag intent={getCategoryColor(project.category)} minimal className="text-xs">
                          {project.category}
                        </Tag>
                        <Tag intent={getStatusColor(project.status)} className="text-xs">
                          {project.status}
                        </Tag>
                      </div>
                      <h3 className="text-lg font-bold text-gray-800 mb-2">{project.title}</h3>
                    </div>
                    <div className="flex items-center gap-1 text-gray-500">
                      <Star size={14} className="fill-yellow-400 text-yellow-400" />
                      <span className="text-xs font-medium">{project.stars}</span>
                    </div>
                  </div>
                  
                  <p className="text-gray-600 text-sm mb-4 flex-1">{project.description}</p>
                  
                  <div className="mb-4">
                    <h4 className="text-sm font-semibold text-gray-800 mb-2">Key Achievements:</h4>
                    <ul className="text-xs text-gray-600 space-y-1">
                      {project.highlights.slice(0, 3).map((highlight, highlightIndex) => (
                        <li key={highlightIndex} className="flex items-start">
                          <span className="text-blue-500 mr-2">•</span>
                          {highlight}
                        </li>
                      ))}
                    </ul>
                  </div>
                  
                  <div className="mb-4">
                    <div className="flex flex-wrap gap-1">
                      {project.technologies.slice(0, 6).map((tech, techIndex) => (
                        <Tag key={techIndex} className="skill-tag text-xs" minimal>
                          {tech}
                        </Tag>
                      ))}
                    </div>
                  </div>
                  
                  <div className="flex gap-2 pt-3 border-t border-gray-200">
                    <a 
                      href={project.github}
                      className="flex items-center gap-1 px-3 py-1.5 bg-gray-800 text-white rounded text-xs hover:bg-gray-700 transition-colors"
                    >
                      <Github size={14} />
                      Code
                    </a>
                    <a 
                      href={project.demo}
                      className="flex items-center gap-1 px-3 py-1.5 bg-orange-600 text-white rounded text-xs hover:bg-orange-700 transition-colors"
                    >
                      <ExternalLink size={14} />
                      Research
                    </a>
                  </div>
                </Card>
              ))}
            </div>
          </div>
        </div>
      </div>
    </section>
  );
};

export default Projects;
