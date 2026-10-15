import { createServer } from 'vite';
import { readFile, writeFile } from 'node:fs/promises';
import { createElement } from 'react';
import { renderToString } from 'react-dom/server';
const server = await createServer({ server: { middlewareMode: true, hmr: false, ws: false }, appType: 'custom' });
try {
  const { default: App } = await server.ssrLoadModule('/src/App.tsx');
  const { applicationSummary } = await server.ssrLoadModule('/src/lib/profile.ts');
  const html = await readFile('dist/index.html', 'utf8');
  const markup = renderToString(createElement(App));
  if (!html.includes('<!--app-html-->')) throw new Error('Missing prerender insertion point');
  const base = 'https://rahuldange.com/';
  const sections = ['about', 'projects', 'experience', 'skills', 'education', 'publications', 'volunteering', 'contact'];
  const labels = ['About', 'Projects', 'Experience', 'Technical Skills', 'Education', 'Publications', 'Volunteering', 'Contact'];
  for (const id of sections) {
    if (!markup.includes(`id="${id}"`)) throw new Error(`Missing crawlable section: ${id}`);
  }
  if ((markup.match(/<h1\b/g) || []).length !== 1) throw new Error('Expected one primary heading');
  if (!markup.includes('href="/resume.v4.pdf"')) throw new Error('Missing crawlable current resume link');
  const schemaMatch = html.match(/<script type="application\/ld\+json">([\s\S]*?)<\/script>/);
  if (!schemaMatch) throw new Error('Missing profile structured data');
  const profile = JSON.parse(schemaMatch[1]);
  const person = {...profile.mainEntity, '@id': `${base}#person`};
  person.description = applicationSummary;
  person.homeLocation = {'@type': 'Place', name: 'Sydney, Australia'};
  profile['@id'] = `${base}#profile`;
  profile.mainEntity = {'@id': person['@id']};
  profile.inLanguage = 'en-AU';
  profile.hasPart = sections.map(id => ({'@id': `${base}#${id}`}));
  profile.about = [{'@id': `${base}#mimfs-project`}, {'@id': `${base}#llfpl-project`}];
  person.subjectOf = [{'@id': `${base}#resume`}, {'@id': `${base}#representation-article`}];
  const graph = {
    '@context': 'https://schema.org',
    '@graph': [profile, person,
      ...sections.map((id, index) => ({
        '@type': 'WebPageElement', '@id': `${base}#${id}`,
        name: labels[index], url: `${base}#${id}`, isPartOf: {'@id': profile['@id']}
      })),
      {
        '@type': 'SoftwareSourceCode', '@id': `${base}#mimfs-project`,
        name: 'MIMFS (Multithreaded In Memory File System)',
        description: 'Thread-safe in-memory POSIX filesystem written in C11, with directories, hard and symbolic links, sparse files, and atomic rename.',
        codeRepository: 'https://github.com/rahuldangeofficial/MIMFS',
        programmingLanguage: 'C11', author: {'@id': person['@id']}
      },
      {
        '@type': 'SoftwareSourceCode', '@id': `${base}#llfpl-project`,
        name: 'LLFPL (Low-Level Floating-Point Language)',
        description: 'C11 interpreter for a deterministic expression language for numerical computing, evaluating expressions directly without constructing a syntax tree.',
        codeRepository: 'https://github.com/rahuldangeofficial/LLFPL',
        programmingLanguage: 'C11', author: {'@id': person['@id']}
      },
      {
        '@type': 'Article', '@id': `${base}#representation-article`,
        headline: 'Beyond the Representation', datePublished: '2026-08-26',
        url: 'https://medium.com/@rahuldangeofficial/beyond-the-representation-e0539a7335b7',
        description: 'An exploratory essay on how problem representation shapes what we can solve, connecting mathematics, AI, and human cognition.',
        author: {'@id': person['@id']}
      },
      {
        '@type': 'DigitalDocument', '@id': `${base}#resume`, name: 'Rahul Dange Resume',
        url: `${base}resume.v4.pdf`, encodingFormat: 'application/pdf', author: {'@id': person['@id']}
      }
    ]
  };
  const structuredData = `<script type="application/ld+json">${JSON.stringify(graph).replace(/</g, '\\u003c')}</script>`;
  const output = html.replace('<!--app-html-->', markup).replace(schemaMatch[0], structuredData);
  await writeFile('dist/index.html', output);
  console.log('Pre-rendered portfolio content into dist/index.html');
} finally { await server.close(); }
