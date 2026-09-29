import { createServer } from 'vite';
import { readFile, writeFile } from 'node:fs/promises';
import { createElement } from 'react';
import { renderToString } from 'react-dom/server';
const server = await createServer({ server: { middlewareMode: true, hmr: false, ws: false }, appType: 'custom' });
try {
  const { default: App } = await server.ssrLoadModule('/src/App.tsx');
  const html = await readFile('dist/index.html', 'utf8');
  const markup = renderToString(createElement(App));
  if (!html.includes('<!--app-html-->')) throw new Error('Missing prerender insertion point');
  await writeFile('dist/index.html', html.replace('<!--app-html-->', markup));
  console.log('Pre-rendered portfolio content into dist/index.html');
} finally { await server.close(); }
