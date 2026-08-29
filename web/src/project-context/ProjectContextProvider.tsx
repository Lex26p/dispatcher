import {
  createContext,
  type ReactNode,
  useCallback,
  useContext,
  useEffect,
  useMemo,
  useState,
} from 'react';

import {
  type Project,
  ProjectManagerClient,
} from '../project-manager/ProjectManagerClient';
import { ServiceHubRequestError } from '../service-hub/ServiceHubClient';
import { useServiceHub } from '../service-hub/ServiceHubProvider';

export const PROJECT_CONTEXT_STORAGE_KEY = 'dispatcher.project-context.v1';

export interface ProjectContextValue {
  readonly selectedProject: Project | null;
  selectProject(project: Project): void;
  clearProject(): void;
}

interface ProjectContextProviderProps {
  readonly children: ReactNode;
}

const ProjectContext = createContext<ProjectContextValue | null>(null);

export function ProjectContextProvider({ children }: ProjectContextProviderProps) {
  const { client, connectionState } = useServiceHub();
  const projectManager = useMemo(() => new ProjectManagerClient(client), [client]);
  const [selectedProject, setSelectedProject] = useState<Project | null>(() =>
    readStoredProject(),
  );

  const selectProject = useCallback((project: Project) => {
    setSelectedProject(project);
  }, []);

  const clearProject = useCallback(() => {
    setSelectedProject(null);
  }, []);

  useEffect(() => {
    persistProject(selectedProject);
  }, [selectedProject]);

  const selectedProjectId = selectedProject?.id ?? null;

  useEffect(() => {
    if (connectionState !== 'connected' || selectedProjectId === null) {
      return;
    }

    let active = true;
    const request = projectManager.getProject(selectedProjectId);

    void request.response
      .then((project) => {
        if (!active) {
          return;
        }

        setSelectedProject((current) =>
          current?.id === selectedProjectId ? project : current,
        );
      })
      .catch((error: unknown) => {
        if (
          active &&
          error instanceof ServiceHubRequestError &&
          error.code === 'project.not_found'
        ) {
          setSelectedProject((current) =>
            current?.id === selectedProjectId ? null : current,
          );
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
  }, [connectionState, projectManager, selectedProjectId]);

  const value = useMemo<ProjectContextValue>(
    () => ({ selectedProject, selectProject, clearProject }),
    [clearProject, selectProject, selectedProject],
  );

  return <ProjectContext.Provider value={value}>{children}</ProjectContext.Provider>;
}

export function useProjectContext(): ProjectContextValue {
  const value = useContext(ProjectContext);

  if (value === null) {
    throw new Error('useProjectContext must be used inside ProjectContextProvider');
  }

  return value;
}

function readStoredProject(): Project | null {
  try {
    const stored = window.sessionStorage.getItem(PROJECT_CONTEXT_STORAGE_KEY);

    if (stored === null) {
      return null;
    }

    const parsed: unknown = JSON.parse(stored);

    if (!isProject(parsed)) {
      window.sessionStorage.removeItem(PROJECT_CONTEXT_STORAGE_KEY);
      return null;
    }

    return parsed;
  } catch {
    return null;
  }
}

function persistProject(project: Project | null): void {
  try {
    if (project === null) {
      window.sessionStorage.removeItem(PROJECT_CONTEXT_STORAGE_KEY);
      return;
    }

    window.sessionStorage.setItem(
      PROJECT_CONTEXT_STORAGE_KEY,
      JSON.stringify(project),
    );
  } catch {
    // Session persistence is best-effort; context remains valid in React state.
  }
}

function isProject(value: unknown): value is Project {
  return (
    typeof value === 'object' &&
    value !== null &&
    !Array.isArray(value) &&
    'id' in value &&
    typeof value.id === 'string' &&
    value.id.length > 0 &&
    'name' in value &&
    typeof value.name === 'string' &&
    'description' in value &&
    typeof value.description === 'string'
  );
}
