import { StrictMode } from 'react';
import { createRoot, hydrateRoot } from 'react-dom/client';
import App from './App.tsx';
import './styles/index.css';

const root = document.getElementById('root')!;
const app = <StrictMode><App /></StrictMode>;
if (root.hasChildNodes() && root.querySelector('main')) {
  hydrateRoot(root, app);
} else {
  createRoot(root).render(app);
}
