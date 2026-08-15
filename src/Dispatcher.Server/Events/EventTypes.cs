namespace Dispatcher.Server.Events;

public static class EventTypes
{
    public const string SystemStarted = "SystemStarted";
    public const string SystemStopping = "SystemStopping";

    public const string DeviceOnline = "DeviceOnline";
    public const string DeviceOffline = "DeviceOffline";

    public const string TagWriteSucceeded = "TagWriteSucceeded";
    public const string TagWriteFailed = "TagWriteFailed";

    public const string RuntimeConfigurationApplied = "RuntimeConfigurationApplied";
}
