import {
  type FormEvent,
  useEffect,
  useMemo,
  useState,
} from 'react';

import { useProjectContext } from '../project-context/ProjectContextProvider';
import {
  ServiceHubRequestError,
  ServiceHubTransportError,
} from '../service-hub/ServiceHubClient';
import { useServiceHub } from '../service-hub/ServiceHubProvider';
import {
  type Project,
  ProjectManagerClient,
  ProjectManagerClientResponseError,
} from './ProjectManagerClient';

interface ProjectEditorState {
  kind: 'create' | 'edit';
  id: string | null;
  name: string;
  description: string;
}

export function ProjectManagerView() {
  const { client, connectionState } = useServiceHub();
  const { selectedProject, selectProject } = useProjectContext();
  const projectManager = useMemo(() => new ProjectManagerClient(client), [client]);
  const [projects, setProjects] = useState<Project[]>([]);
  const [loading, setLoading] = useState(false);
  const [listError, setListError] = useState<string | null>(null);
  const [reloadNumber, setReloadNumber] = useState(0);
  const [editor, setEditor] = useState<ProjectEditorState | null>(null);
  const [editorError, setEditorError] = useState<string | null>(null);
  const [saving, setSaving] = useState(false);

  useEffect(() => {
    if (connectionState !== 'connected') {
      setLoading(false);
      return;
    }

    let active = true;
    const request = projectManager.listProjects();

    setLoading(true);
    setListError(null);

    void request.response
      .then((result) => {
        if (active) {
          setProjects(result);
        }
      })
      .catch((error: unknown) => {
        if (active) {
          setListError(projectManagerErrorMessage(error));
        }
      })
      .finally(() => {
        if (active) {
          setLoading(false);
        }
      });

    return () => {
      active = false;

      try {
        request.cancel();
      } catch {
        // Connection teardown already resolves the pending transport state.
      }
    };
  }, [connectionState, projectManager, reloadNumber]);

  const openCreate = () => {
    setEditor({
      kind: 'create',
      id: null,
      name: '',
      description: '',
    });
    setEditorError(null);
  };

  const openEdit = (project: Project) => {
    setEditor({
      kind: 'edit',
      id: project.id,
      name: project.name,
      description: project.description,
    });
    setEditorError(null);
  };

  const closeEditor = () => {
    if (saving) {
      return;
    }

    setEditor(null);
    setEditorError(null);
  };

  const saveProject = async (event: FormEvent<HTMLFormElement>) => {
    event.preventDefault();

    if (editor === null || connectionState !== 'connected') {
      setEditorError('Service Hub недоступен. Сохранение проекта невозможно.');
      return;
    }

    setSaving(true);
    setEditorError(null);

    try {
      const request =
        editor.kind === 'create'
          ? projectManager.createProject({
              name: editor.name,
              description: editor.description,
            })
          : projectManager.updateProject({
              id: editor.id ?? '',
              name: editor.name,
              description: editor.description,
            });

      const savedProject = await request.response;

      setProjects((current) => {
        if (editor.kind === 'create') {
          return [...current, savedProject];
        }

        return current.map((project) =>
          project.id === savedProject.id ? savedProject : project,
        );
      });

      if (selectedProject?.id === savedProject.id) {
        selectProject(savedProject);
      }

      setEditor(null);
    } catch (error) {
      setEditorError(projectManagerErrorMessage(error));
    } finally {
      setSaving(false);
    }
  };

  return (
    <section className="workspace__content project-manager" aria-labelledby="projects-title">
      <div className="project-manager__header">
        <div>
          <p className="workspace__eyebrow">Project Manager</p>
          <h1 id="projects-title">Проекты</h1>
          <p className="workspace__description">
            Создавайте проекты и изменяйте их базовые свойства.
          </p>
        </div>

        {editor === null ? (
          <button
            className="project-action project-action--primary"
            type="button"
            disabled={connectionState !== 'connected' || loading}
            onClick={openCreate}
          >
            Создать проект
          </button>
        ) : null}
      </div>

      {editor !== null ? (
        <form className="project-editor" onSubmit={saveProject}>
          <div className="project-editor__heading">
            <div>
              <h2>{editor.kind === 'create' ? 'Новый проект' : 'Редактирование проекта'}</h2>
              {editor.id !== null ? (
                <p className="project-editor__id">ID: {editor.id}</p>
              ) : null}
            </div>
          </div>

          {connectionState !== 'connected' ? (
            <div className="project-notice project-notice--error" role="alert">
              Service Hub недоступен. Изменения нельзя сохранить.
            </div>
          ) : null}

          {editorError !== null ? (
            <div className="project-notice project-notice--error" role="alert">
              {editorError}
            </div>
          ) : null}

          <label className="project-field">
            <span>Название</span>
            <input
              name="project-name"
              value={editor.name}
              required
              autoFocus
              disabled={saving}
              onChange={(event) =>
                setEditor((current) =>
                  current === null
                    ? current
                    : { ...current, name: event.target.value },
                )
              }
            />
          </label>

          <label className="project-field">
            <span>Описание</span>
            <textarea
              name="project-description"
              rows={5}
              value={editor.description}
              disabled={saving}
              onChange={(event) =>
                setEditor((current) =>
                  current === null
                    ? current
                    : { ...current, description: event.target.value },
                )
              }
            />
          </label>

          <div className="project-editor__actions">
            <button
              className="project-action project-action--primary"
              type="submit"
              disabled={saving || connectionState !== 'connected'}
            >
              {saving ? 'Сохранение…' : 'Сохранить'}
            </button>
            <button
              className="project-action"
              type="button"
              disabled={saving}
              onClick={closeEditor}
            >
              Отмена
            </button>
          </div>
        </form>
      ) : (
        <div className="project-list-view">
          {connectionState !== 'connected' ? (
            <div className="project-notice project-notice--error" role="alert">
              Service Hub недоступен. Список проектов сейчас нельзя загрузить.
            </div>
          ) : loading ? (
            <p className="project-list-view__status" role="status">
              Загрузка проектов…
            </p>
          ) : listError !== null ? (
            <div className="project-notice project-notice--error" role="alert">
              <span>{listError}</span>
              <button
                className="project-action"
                type="button"
                onClick={() => setReloadNumber((value) => value + 1)}
              >
                Повторить
              </button>
            </div>
          ) : projects.length === 0 ? (
            <div className="project-empty">
              <h2>Проектов пока нет</h2>
              <p>Создайте первый проект, чтобы начать работу с Project Manager.</p>
            </div>
          ) : (
            <ul className="project-list" aria-label="Список проектов">
              {projects.map((project) => {
                const selected = selectedProject?.id === project.id;

                return (
                  <li
                    key={project.id}
                    className={`project-list__entry${
                      selected ? ' project-list__entry--selected' : ''
                    }`}
                  >
                    <button
                      className="project-list__item"
                      type="button"
                      onClick={() => openEdit(project)}
                    >
                      <strong>{project.name}</strong>
                      <span>{project.description || 'Без описания'}</span>
                    </button>
                    <button
                      className="project-action project-action--context"
                      type="button"
                      disabled={selected}
                      aria-label={
                        selected
                          ? `${project.name}: текущий контекст`
                          : `Выбрать ${project.name} как текущий контекст`
                      }
                      onClick={() => selectProject(project)}
                    >
                      {selected ? 'Текущий' : 'Выбрать контекст'}
                    </button>
                  </li>
                );
              })}
            </ul>
          )}
        </div>
      )}
    </section>
  );
}

function projectManagerErrorMessage(error: unknown): string {
  if (error instanceof ProjectManagerClientResponseError) {
    return 'Project Manager вернул некорректный ответ.';
  }

  if (error instanceof ServiceHubTransportError) {
    return 'Service Hub недоступен. Не удалось выполнить запрос к Project Manager.';
  }

  if (error instanceof ServiceHubRequestError) {
    switch (error.code) {
      case 'hub.unknown_service':
      case 'hub.provider_unavailable':
        return 'Project Manager недоступен.';
      case 'hub.timeout':
        return 'Project Manager не ответил вовремя.';
      case 'hub.cancelled':
        return 'Запрос к Project Manager был отменён.';
      case 'project.invalid_name':
        return 'Укажите непустое название проекта.';
      case 'project.name_too_long':
        return 'Название проекта слишком длинное.';
      case 'project.description_too_long':
        return 'Описание проекта слишком длинное.';
      case 'project.not_found':
        return 'Проект больше не существует.';
      case 'project.storage_error':
        return 'Project Manager не смог сохранить данные.';
      case 'project.invalid_request':
        return 'Project Manager отклонил некорректный запрос.';
      default:
        return `Project Manager вернул ошибку: ${error.code}.`;
    }
  }

  return 'Не удалось выполнить запрос к Project Manager.';
}
