import { render, screen } from '@testing-library/react';

import { App } from './App';

describe('App bootstrap', () => {
  it('renders the Web Shell bootstrap page', () => {
    render(<App />);

    expect(
      screen.getByRole('heading', { name: 'Диспетчер Web Shell' }),
    ).toBeInTheDocument();

    expect(
      screen.getByText(
        'Frontend-каркас React + TypeScript готов к следующему шагу.',
      ),
    ).toBeInTheDocument();
  });
});
