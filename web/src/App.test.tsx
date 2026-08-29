import { fireEvent, render, screen, within } from '@testing-library/react';

import { App } from './App';

describe('App Shell navigation', () => {
  beforeEach(() => {
    window.history.replaceState(null, '', '/');
  });

  it('opens and closes the global menu with keyboard-friendly focus behavior', () => {
    render(<App />);

    const menuTrigger = screen.getByRole('button', { name: 'Основное меню' });

    expect(menuTrigger).toBeEnabled();
    expect(menuTrigger).toHaveAttribute('aria-expanded', 'false');
    expect(
      screen.getByRole('heading', { name: 'Рабочая область' }),
    ).toBeInTheDocument();

    fireEvent.click(menuTrigger);

    const navigation = screen.getByRole('navigation', {
      name: 'Глобальная навигация',
    });
    const workspaceLink = within(navigation).getByRole('link', {
      name: 'Рабочая область',
    });

    expect(menuTrigger).toHaveAttribute('aria-expanded', 'true');
    expect(workspaceLink).toHaveAttribute('aria-current', 'page');
    expect(within(navigation).getAllByRole('link')).toHaveLength(1);
    expect(workspaceLink).toHaveFocus();

    fireEvent.keyDown(document, { key: 'Escape' });

    expect(
      screen.queryByRole('navigation', { name: 'Глобальная навигация' }),
    ).not.toBeInTheDocument();
    expect(menuTrigger).toHaveAttribute('aria-expanded', 'false');
    expect(menuTrigger).toHaveFocus();

    fireEvent.click(menuTrigger);
    fireEvent.click(menuTrigger);

    expect(
      screen.queryByRole('navigation', { name: 'Глобальная навигация' }),
    ).not.toBeInTheDocument();
  });

  it('shows an unknown-route fallback and returns to the shell workspace', () => {
    window.history.replaceState(null, '', '/missing');
    render(<App />);

    expect(
      screen.getByRole('heading', { name: 'Страница не найдена' }),
    ).toBeInTheDocument();

    const menuTrigger = screen.getByRole('button', { name: 'Основное меню' });
    fireEvent.click(menuTrigger);

    const navigation = screen.getByRole('navigation', {
      name: 'Глобальная навигация',
    });
    const workspaceLink = within(navigation).getByRole('link', {
      name: 'Рабочая область',
    });

    expect(workspaceLink).not.toHaveAttribute('aria-current');

    fireEvent.click(workspaceLink);

    expect(window.location.pathname).toBe('/');
    expect(
      screen.getByRole('heading', { name: 'Рабочая область' }),
    ).toBeInTheDocument();
    expect(
      screen.queryByRole('navigation', { name: 'Глобальная навигация' }),
    ).not.toBeInTheDocument();
  });
});
