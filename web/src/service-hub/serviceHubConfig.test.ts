import { resolveServiceHubUrl } from './serviceHubConfig';

describe('resolveServiceHubUrl', () => {
  it('uses an explicitly configured Service Hub URL', () => {
    expect(
      resolveServiceHubUrl('  ws://127.0.0.1:8090/v1/ws  ', {
        protocol: 'https:',
        host: 'dispatcher.example',
      }),
    ).toBe('ws://127.0.0.1:8090/v1/ws');
  });

  it('derives a same-origin ws URL for an http page', () => {
    expect(
      resolveServiceHubUrl(undefined, {
        protocol: 'http:',
        host: '127.0.0.1:4173',
      }),
    ).toBe('ws://127.0.0.1:4173/v1/ws');
  });

  it('derives a same-origin wss URL for an https page', () => {
    expect(
      resolveServiceHubUrl(undefined, {
        protocol: 'https:',
        host: 'dispatcher.example',
      }),
    ).toBe('wss://dispatcher.example/v1/ws');
  });
});
