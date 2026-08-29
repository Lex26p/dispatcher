const serviceHubEndpointPath = '/v1/ws';

export function resolveServiceHubUrl(
  configuredUrl: string | undefined = import.meta.env.VITE_SERVICE_HUB_URL,
  location: Pick<Location, 'protocol' | 'host'> = window.location,
): string {
  const explicitUrl = configuredUrl?.trim();

  if (explicitUrl) {
    return explicitUrl;
  }

  const webSocketProtocol = location.protocol === 'https:' ? 'wss:' : 'ws:';
  return `${webSocketProtocol}//${location.host}${serviceHubEndpointPath}`;
}
