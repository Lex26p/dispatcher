namespace Dispatcher.Server.Alarms;

public enum AlarmRuntimeState
{
    Normal,
    ActiveUnacknowledged,
    ActiveAcknowledged,
    ReturnedUnacknowledged
}
