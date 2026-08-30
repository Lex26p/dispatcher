import { StrictMode } from 'react';
import { createRoot } from 'react-dom/client';

import { App } from './App';
import { ProjectContextProvider } from './project-context/ProjectContextProvider';
import { ServiceHubClient } from './service-hub/ServiceHubClient';
import { ServiceHubProvider } from './service-hub/ServiceHubProvider';
import { resolveServiceHubUrl } from './service-hub/serviceHubConfig';
import {
  BrowserSessionServiceHubClient,
  BrowserSessionStore,
} from './user-session/BrowserSessionTransport';
import { UserSessionProvider } from './user-session/UserSessionProvider';
import './styles.css';

const rootElement = document.getElementById('root');

if (rootElement === null) {
  throw new Error('Web Shell root element was not found');
}

const baseServiceHubClient = new ServiceHubClient({
  url: resolveServiceHubUrl(),
});
const browserSessionStore = new BrowserSessionStore();
const serviceHubClient = new BrowserSessionServiceHubClient(
  baseServiceHubClient,
  browserSessionStore,
);

if (import.meta.env.VITE_SERVICE_HUB_E2E === '1') {
  Object.defineProperty(window, '__dispatcherServiceHubE2EClient', {
    value: serviceHubClient,
    configurable: true,
  });
}

createRoot(rootElement).render(
  <ServiceHubProvider client={serviceHubClient}>
    <UserSessionProvider sessionStore={browserSessionStore}>
      <ProjectContextProvider>
        <StrictMode>
          <App />
        </StrictMode>
      </ProjectContextProvider>
    </UserSessionProvider>
  </ServiceHubProvider>,
);
