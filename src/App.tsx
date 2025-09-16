import React, { Suspense, lazy } from 'react';
import Header from './components/Header';
import LoadingSpinner from './components/LoadingSpinner';

// Lazy load components for better performance
const About = lazy(() => import('./components/About'));
const Skills = lazy(() => import('./components/Skills'));
const Projects = lazy(() => import('./components/Projects'));
const Education = lazy(() => import('./components/Education'));
const TechnicalPreferences = lazy(() => import('./components/TechnicalPreferences'));
const PersonalInterests = lazy(() => import('./components/PersonalInterests'));
const BlogArticles = lazy(() => import('./components/BlogArticles'));
const Contact = lazy(() => import('./components/Contact'));

function App() {
  return (
    <div className="min-h-screen">
      <Header />
      <Suspense fallback={<LoadingSpinner />}>
        <About />
      </Suspense>
      <Suspense fallback={<LoadingSpinner />}>
        <Skills />
      </Suspense>
      <Suspense fallback={<LoadingSpinner />}>
        <Projects />
      </Suspense>
      <Suspense fallback={<LoadingSpinner />}>
        <Education />
      </Suspense>
      <Suspense fallback={<LoadingSpinner />}>
        <TechnicalPreferences />
      </Suspense>
      <Suspense fallback={<LoadingSpinner />}>
        <PersonalInterests />
      </Suspense>
      <Suspense fallback={<LoadingSpinner />}>
        <BlogArticles />
      </Suspense>
      <Suspense fallback={<LoadingSpinner />}>
        <Contact />
      </Suspense>
    </div>
  );
}

export default App;