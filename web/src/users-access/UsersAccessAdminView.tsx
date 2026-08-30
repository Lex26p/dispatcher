import { type FormEvent, useEffect, useMemo, useState } from 'react';

import { useServiceHub } from '../service-hub/ServiceHubProvider';
import {
  useUserSession,
  userSessionErrorMessage,
} from '../user-session/UserSessionProvider';
import {
  type AccessAssignment,
  type CapabilityName,
  type PermissionSet,
  type User,
  UsersAccessClient,
} from './UsersAccessClient';

const capabilityLabels: Record<CapabilityName, string> = {
  view: 'Просмотр',
  control: 'Управление',
  edit: 'Редактирование',
  admin: 'Администрирование',
};

export function UsersAccessAdminView() {
  const { client, connectionState } = useServiceHub();
  const { hasGlobalCapability } = useUserSession();
  const usersAccess = useMemo(() => new UsersAccessClient(client), [client]);
  const [users, setUsers] = useState<User[]>([]);
  const [permissionSets, setPermissionSets] = useState<PermissionSet[]>([]);
  const [assignments, setAssignments] = useState<AccessAssignment[]>([]);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [reloadNumber, setReloadNumber] = useState(0);

  const [newLogin, setNewLogin] = useState('');
  const [newDisplayName, setNewDisplayName] = useState('');
  const [newPassword, setNewPassword] = useState('');
  const [newEnabled, setNewEnabled] = useState(true);
  const [creatingUser, setCreatingUser] = useState(false);

  const [passwordTargetId, setPasswordTargetId] = useState<string | null>(null);
  const [resetPassword, setResetPassword] = useState('');
  const [savingPassword, setSavingPassword] = useState(false);

  const [permissionName, setPermissionName] = useState('');
  const [permissionCapabilities, setPermissionCapabilities] = useState<CapabilityName[]>([]);
  const [creatingPermissionSet, setCreatingPermissionSet] = useState(false);

  const [assignmentUserId, setAssignmentUserId] = useState('');
  const [assignmentPermissionSetId, setAssignmentPermissionSetId] = useState('');
  const [assignmentScopeKind, setAssignmentScopeKind] = useState<'global' | 'project'>('global');
  const [assignmentProjectId, setAssignmentProjectId] = useState('');
  const [savingAssignment, setSavingAssignment] = useState(false);

  const isAdmin = hasGlobalCapability('admin');

  useEffect(() => {
    if (connectionState !== 'connected' || !isAdmin) {
      setLoading(false);
      return;
    }

    let active = true;
    const usersRequest = usersAccess.listUsers();
    const permissionSetsRequest = usersAccess.listPermissionSets();
    const assignmentsRequest = usersAccess.listAccessAssignments();
    const requests = [usersRequest, permissionSetsRequest, assignmentsRequest] as const;

    setLoading(true);
    setError(null);

    void Promise.all([
      usersRequest.response,
      permissionSetsRequest.response,
      assignmentsRequest.response,
    ])
      .then(([nextUsers, nextPermissionSets, nextAssignments]) => {
        if (!active) {
          return;
        }
        setUsers(nextUsers);
        setPermissionSets(nextPermissionSets);
        setAssignments(nextAssignments);
        setAssignmentUserId((current) => current || nextUsers[0]?.id || '');
        setAssignmentPermissionSetId(
          (current) => current || nextPermissionSets[0]?.id || '',
        );
      })
      .catch((requestError: unknown) => {
        if (active) {
          setError(userSessionErrorMessage(requestError));
        }
      })
      .finally(() => {
        if (active) {
          setLoading(false);
        }
      });

    return () => {
      active = false;
      for (const request of requests) {
        try {
          request.cancel();
        } catch {
          // Connection teardown already resolves pending transport state.
        }
      }
    };
  }, [connectionState, isAdmin, reloadNumber, usersAccess]);

  const createUser = async (event: FormEvent<HTMLFormElement>) => {
    event.preventDefault();
    setCreatingUser(true);
    setError(null);
    try {
      const request = usersAccess.createUser({
        login: newLogin,
        displayName: newDisplayName,
        enabled: newEnabled,
        password: newPassword,
      });
      const user = await request.response;
      setUsers((current) => [...current, user]);
      setNewLogin('');
      setNewDisplayName('');
      setNewPassword('');
      setNewEnabled(true);
      setAssignmentUserId((current) => current || user.id);
    } catch (requestError) {
      setError(userSessionErrorMessage(requestError));
    } finally {
      setCreatingUser(false);
    }
  };

  const toggleUser = async (user: User) => {
    setError(null);
    try {
      const request = usersAccess.setUserEnabled(user.id, !user.enabled);
      const updated = await request.response;
      setUsers((current) =>
        current.map((candidate) => candidate.id === updated.id ? updated : candidate),
      );
    } catch (requestError) {
      setError(userSessionErrorMessage(requestError));
    }
  };

  const savePassword = async (event: FormEvent<HTMLFormElement>) => {
    event.preventDefault();
    if (passwordTargetId === null) {
      return;
    }
    setSavingPassword(true);
    setError(null);
    try {
      await usersAccess.setUserPassword(passwordTargetId, resetPassword).response;
      setPasswordTargetId(null);
      setResetPassword('');
    } catch (requestError) {
      setError(userSessionErrorMessage(requestError));
    } finally {
      setSavingPassword(false);
    }
  };

  const toggleCapability = (capability: CapabilityName) => {
    setPermissionCapabilities((current) =>
      current.includes(capability)
        ? current.filter((candidate) => candidate !== capability)
        : [...current, capability],
    );
  };

  const createPermissionSet = async (event: FormEvent<HTMLFormElement>) => {
    event.preventDefault();
    setCreatingPermissionSet(true);
    setError(null);
    try {
      const request = usersAccess.createPermissionSet({
        name: permissionName,
        capabilities: permissionCapabilities,
      });
      const permissionSet = await request.response;
      setPermissionSets((current) => [...current, permissionSet]);
      setPermissionName('');
      setPermissionCapabilities([]);
      setAssignmentPermissionSetId((current) => current || permissionSet.id);
    } catch (requestError) {
      setError(userSessionErrorMessage(requestError));
    } finally {
      setCreatingPermissionSet(false);
    }
  };

  const addAssignment = async (event: FormEvent<HTMLFormElement>) => {
    event.preventDefault();
    if (assignmentUserId === '' || assignmentPermissionSetId === '') {
      return;
    }

    setSavingAssignment(true);
    setError(null);
    try {
      const assignment: AccessAssignment = {
        user_id: assignmentUserId,
        permission_set_id: assignmentPermissionSetId,
        scope:
          assignmentScopeKind === 'global'
            ? { kind: 'global' }
            : { kind: 'project', project_id: assignmentProjectId },
      };
      const saved = await usersAccess.assignAccess(assignment).response;
      setAssignments((current) => [...current, saved]);
      setAssignmentProjectId('');
    } catch (requestError) {
      setError(userSessionErrorMessage(requestError));
    } finally {
      setSavingAssignment(false);
    }
  };

  const removeAssignment = async (assignment: AccessAssignment) => {
    setError(null);
    try {
      await usersAccess.removeAccessAssignment(assignment).response;
      setAssignments((current) => current.filter((candidate) =>
        !sameAssignment(candidate, assignment),
      ));
    } catch (requestError) {
      setError(userSessionErrorMessage(requestError));
    }
  };

  if (!isAdmin) {
    return (
      <section className="workspace__content access-admin" aria-labelledby="access-title">
        <p className="workspace__eyebrow">Users &amp; Access</p>
        <h1 id="access-title">Пользователи и доступ</h1>
        <div className="auth-notice auth-notice--error" role="alert">
          Для администрирования требуется global capability admin.
        </div>
      </section>
    );
  }

  return (
    <section className="workspace__content access-admin" aria-labelledby="access-title">
      <div className="access-admin__header">
        <div>
          <p className="workspace__eyebrow">Users &amp; Access</p>
          <h1 id="access-title">Пользователи и доступ</h1>
          <p className="workspace__description">
            Локальные пользователи, наборы прав и global/project assignments.
          </p>
        </div>
        <button
          className="project-action"
          type="button"
          disabled={loading || connectionState !== 'connected'}
          onClick={() => setReloadNumber((value) => value + 1)}
        >
          Обновить
        </button>
      </div>

      {connectionState !== 'connected' ? (
        <div className="auth-notice auth-notice--error" role="alert">
          Service Hub недоступен. Администрирование сейчас невозможно.
        </div>
      ) : null}
      {error !== null ? (
        <div className="auth-notice auth-notice--error" role="alert">{error}</div>
      ) : null}
      {loading ? <p className="auth-status" role="status">Загрузка Users &amp; Access…</p> : null}

      <div className="access-admin__grid">
        <section className="access-card" aria-labelledby="users-heading">
          <h2 id="users-heading">Пользователи</h2>
          <form className="access-form" onSubmit={createUser}>
            <label className="project-field">
              <span>Логин</span>
              <input required value={newLogin} onChange={(event) => setNewLogin(event.target.value)} />
            </label>
            <label className="project-field">
              <span>Отображаемое имя</span>
              <input value={newDisplayName} onChange={(event) => setNewDisplayName(event.target.value)} />
            </label>
            <label className="project-field">
              <span>Начальный пароль</span>
              <input
                type="password"
                minLength={15}
                maxLength={1024}
                required
                autoComplete="new-password"
                value={newPassword}
                onChange={(event) => setNewPassword(event.target.value)}
              />
            </label>
            <label className="access-check">
              <input
                type="checkbox"
                checked={newEnabled}
                onChange={(event) => setNewEnabled(event.target.checked)}
              />
              <span>Пользователь включён</span>
            </label>
            <button className="project-action project-action--primary" type="submit" disabled={creatingUser}>
              {creatingUser ? 'Создание…' : 'Создать пользователя'}
            </button>
          </form>

          <ul className="access-list" aria-label="Пользователи">
            {users.map((user) => (
              <li key={user.id} className="access-list__item">
                <div className="access-list__content">
                  <strong>{user.display_name || user.login}</strong>
                  <span>{user.login} · {user.enabled ? 'включён' : 'отключён'}</span>
                  <code>{user.id}</code>
                </div>
                <div className="auth-inline-actions">
                  <button className="project-action" type="button" onClick={() => void toggleUser(user)}>
                    {user.enabled ? 'Отключить' : 'Включить'}
                  </button>
                  <button
                    className="project-action"
                    type="button"
                    onClick={() => {
                      setPasswordTargetId(user.id);
                      setResetPassword('');
                    }}
                  >
                    Сменить пароль
                  </button>
                </div>
              </li>
            ))}
          </ul>

          {passwordTargetId !== null ? (
            <form className="access-inline-form" onSubmit={savePassword}>
              <label className="project-field">
                <span>Новый пароль</span>
                <input
                  type="password"
                  minLength={15}
                  maxLength={1024}
                  required
                  autoComplete="new-password"
                  value={resetPassword}
                  onChange={(event) => setResetPassword(event.target.value)}
                />
              </label>
              <div className="auth-inline-actions">
                <button className="project-action project-action--primary" type="submit" disabled={savingPassword}>
                  {savingPassword ? 'Сохранение…' : 'Сохранить пароль'}
                </button>
                <button
                  className="project-action"
                  type="button"
                  disabled={savingPassword}
                  onClick={() => setPasswordTargetId(null)}
                >
                  Отмена
                </button>
              </div>
            </form>
          ) : null}
        </section>

        <section className="access-card" aria-labelledby="permissions-heading">
          <h2 id="permissions-heading">Наборы прав</h2>
          <form className="access-form" onSubmit={createPermissionSet}>
            <label className="project-field">
              <span>Название набора</span>
              <input required value={permissionName} onChange={(event) => setPermissionName(event.target.value)} />
            </label>
            <fieldset className="access-capabilities">
              <legend>Capabilities</legend>
              {(Object.keys(capabilityLabels) as CapabilityName[]).map((capability) => (
                <label key={capability} className="access-check">
                  <input
                    type="checkbox"
                    checked={permissionCapabilities.includes(capability)}
                    onChange={() => toggleCapability(capability)}
                  />
                  <span>{capabilityLabels[capability]} ({capability})</span>
                </label>
              ))}
            </fieldset>
            <button className="project-action project-action--primary" type="submit" disabled={creatingPermissionSet}>
              {creatingPermissionSet ? 'Создание…' : 'Создать набор прав'}
            </button>
          </form>

          <ul className="access-list" aria-label="Наборы прав">
            {permissionSets.map((permissionSet) => (
              <li key={permissionSet.id} className="access-list__item">
                <div className="access-list__content">
                  <strong>{permissionSet.name}</strong>
                  <span>{permissionSet.capabilities.length === 0 ? 'Без capabilities' : permissionSet.capabilities.join(', ')}</span>
                  <code>{permissionSet.id}</code>
                </div>
              </li>
            ))}
          </ul>
        </section>
      </div>

      <section className="access-card" aria-labelledby="assignments-heading">
        <h2 id="assignments-heading">Назначения доступа</h2>
        <form className="assignment-form" onSubmit={addAssignment}>
          <label className="project-field">
            <span>Пользователь</span>
            <select required value={assignmentUserId} onChange={(event) => setAssignmentUserId(event.target.value)}>
              <option value="">Выберите пользователя</option>
              {users.map((user) => <option key={user.id} value={user.id}>{user.display_name || user.login}</option>)}
            </select>
          </label>
          <label className="project-field">
            <span>Набор прав</span>
            <select required value={assignmentPermissionSetId} onChange={(event) => setAssignmentPermissionSetId(event.target.value)}>
              <option value="">Выберите набор</option>
              {permissionSets.map((permissionSet) => <option key={permissionSet.id} value={permissionSet.id}>{permissionSet.name}</option>)}
            </select>
          </label>
          <label className="project-field">
            <span>Scope</span>
            <select value={assignmentScopeKind} onChange={(event) => setAssignmentScopeKind(event.target.value as 'global' | 'project')}>
              <option value="global">Global</option>
              <option value="project">Project</option>
            </select>
          </label>
          {assignmentScopeKind === 'project' ? (
            <label className="project-field">
              <span>Project ID</span>
              <input required value={assignmentProjectId} onChange={(event) => setAssignmentProjectId(event.target.value)} />
            </label>
          ) : null}
          <button
            className="project-action project-action--primary"
            type="submit"
            disabled={savingAssignment || users.length === 0 || permissionSets.length === 0}
          >
            {savingAssignment ? 'Назначение…' : 'Добавить назначение'}
          </button>
        </form>

        <ul className="access-list" aria-label="Назначения доступа">
          {assignments.map((assignment) => (
            <li key={assignmentKey(assignment)} className="access-list__item">
              <div className="access-list__content">
                <strong>{userName(users, assignment.user_id)}</strong>
                <span>{permissionSetName(permissionSets, assignment.permission_set_id)}</span>
                <code>{scopeLabel(assignment)}</code>
              </div>
              <button className="project-action" type="button" onClick={() => void removeAssignment(assignment)}>
                Удалить
              </button>
            </li>
          ))}
        </ul>
      </section>
    </section>
  );
}

function assignmentKey(assignment: AccessAssignment): string {
  return `${assignment.user_id}|${assignment.permission_set_id}|${scopeLabel(assignment)}`;
}

function scopeLabel(assignment: AccessAssignment): string {
  return assignment.scope.kind === 'global'
    ? 'global'
    : `project:${assignment.scope.project_id}`;
}

function sameAssignment(left: AccessAssignment, right: AccessAssignment): boolean {
  return assignmentKey(left) === assignmentKey(right);
}

function userName(users: User[], userId: string): string {
  const user = users.find((candidate) => candidate.id === userId);
  return user === undefined ? userId : user.display_name || user.login;
}

function permissionSetName(permissionSets: PermissionSet[], permissionSetId: string): string {
  return permissionSets.find((candidate) => candidate.id === permissionSetId)?.name ?? permissionSetId;
}
