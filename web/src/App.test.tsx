import { render, screen } from '@testing-library/react';

import { App } from './App';

describe('App Shell layout', () => {
  it('renders the global header and main workspace without future actions', () => {
    render(<App />);

    expect(screen.getByRole('banner')).toBeInTheDocument();
    expect(screen.getByText('Диспетчер')).toBeInTheDocument();

    const menuTrigger = screen.getByRole('button', { name: 'Основное меню' });
    expect(menuTrigger).toBeDisabled();

    expect(
      screen.getByRole('group', { name: 'Область глобальных действий' }),
    ).toBeInTheDocument();

    expect(
      screen.getByRole('heading', { name: 'Рабочая область' }),
    ).toBeInTheDocument();

    expect(screen.getAllByRole('button')).toHaveLength(1);
  });
});
