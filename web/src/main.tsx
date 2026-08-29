import { StrictMode } from 'react';
import { createRoot } from 'react-dom/client';

import { App } from './App';
import { ServiceHubClient } from './service-hub/ServiceHubClient';
import { ServiceHubProvider } from './service-hub/ServiceHubProvider';
import { resolveServiceHubUrl } from './service-hub/serviceHubConfig';
import './styles.css';

const rootElement = document.getElementById('root');

if (rootElement === null) {
  throw new Error('Web Shell root element was not found');
}

const serviceHubClient = new ServiceHubClient({
  url: resolveServiceHubUrl(),
});

if (import.meta.env.VITE_SERVICE_HUB_E2E === '1') {
  Object.defineProperty(window, '__dispatcherServiceHubE2EClient', {
    value: serviceHubClient,
    configurable: true,
  });
}

createRoot(rootElement).render(
  <ServiceHubProvider client={serviceHubClient}>
    <StrictMode>
      <App />
    </StrictMode>
  </ServiceHubProvider>,
);
