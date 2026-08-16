namespace Dispatcher.Server.Alarms;

public sealed class AlarmDefinitionNotFoundException(
    string message)
    : InvalidOperationException(message);

public sealed class AlarmDefinitionConflictException(
    string message)
    : InvalidOperationException(message);
