import { type FormEvent, useState } from 'react';

import { useServiceHub } from '../service-hub/ServiceHubProvider';
import {
  useUserSession,
  userSessionErrorMessage,
} from '../user-session/UserSessionProvider';

interface LoginViewProps {
  readonly title?: string;
  readonly description?: string;
  readonly onAuthenticated?: () => void;
}

export function LoginView({
  title = 'Вход',
  description = 'Войдите под локальной учётной записью Dispatcher.',
  onAuthenticated,
}: LoginViewProps) {
  const { connectionState } = useServiceHub();
  const {
    status,
    session,
    restorationError,
    login,
    forgetSession,
    retryRestoration,
  } = useUserSession();
  const [loginValue, setLoginValue] = useState('');
  const [password, setPassword] = useState('');
  const [submitting, setSubmitting] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const submit = async (event: FormEvent<HTMLFormElement>) => {
    event.preventDefault();
    if (connectionState !== 'connected') {
      setError('Service Hub недоступен. Вход сейчас невозможен.');
      return;
    }

    setSubmitting(true);
    setError(null);

    try {
      await login(loginValue, password);
      setPassword('');
      onAuthenticated?.();
    } catch (requestError) {
      setError(userSessionErrorMessage(requestError));
    } finally {
      setSubmitting(false);
    }
  };

  if (status === 'authenticated' && session !== null) {
    return (
      <section className="workspace__content auth-view" aria-labelledby="login-title">
        <p className="workspace__eyebrow">Users &amp; Access</p>
        <h1 id="login-title">{title}</h1>
        <p className="workspace__description">
          Вы вошли как <strong>{session.user.display_name || session.user.login}</strong>.
        </p>
      </section>
    );
  }

  if (status === 'restoring') {
    return (
      <section className="workspace__content auth-view" aria-labelledby="login-title">
        <p className="workspace__eyebrow">Users &amp; Access</p>
        <h1 id="login-title">Проверка сессии</h1>
        <p className="workspace__description">
          Проверяем сохранённую browser session через Users &amp; Access.
        </p>
        {restorationError !== null ? (
          <div className="auth-notice auth-notice--error" role="alert">
            <span>{restorationError}</span>
            <div className="auth-inline-actions">
              <button
                className="project-action"
                type="button"
                disabled={connectionState !== 'connected'}
                onClick={retryRestoration}
              >
                Повторить
              </button>
              <button
                className="project-action"
                type="button"
                onClick={forgetSession}
              >
                Войти заново
              </button>
            </div>
          </div>
        ) : (
          <p className="auth-status" role="status">Проверка пользователя…</p>
        )}
      </section>
    );
  }

  return (
    <section className="workspace__content auth-view" aria-labelledby="login-title">
      <p className="workspace__eyebrow">Users &amp; Access</p>
      <h1 id="login-title">{title}</h1>
      <p className="workspace__description">{description}</p>

      {connectionState !== 'connected' ? (
        <div className="auth-notice auth-notice--error" role="alert">
          Service Hub недоступен. Вход сейчас невозможен.
        </div>
      ) : null}

      {error !== null ? (
        <div className="auth-notice auth-notice--error" role="alert">
          {error}
        </div>
      ) : null}

      <form className="auth-form" onSubmit={submit}>
        <label className="project-field">
          <span>Логин</span>
          <input
            name="login"
            autoComplete="username"
            required
            autoFocus
            disabled={submitting}
            value={loginValue}
            onChange={(event) => setLoginValue(event.target.value)}
          />
        </label>

        <label className="project-field">
          <span>Пароль</span>
          <input
            name="password"
            type="password"
            autoComplete="current-password"
            required
            disabled={submitting}
            value={password}
            onChange={(event) => setPassword(event.target.value)}
          />
        </label>

        <button
          className="project-action project-action--primary"
          type="submit"
          disabled={submitting || connectionState !== 'connected'}
        >
          {submitting ? 'Вход…' : 'Войти'}
        </button>
      </form>
    </section>
  );
}
