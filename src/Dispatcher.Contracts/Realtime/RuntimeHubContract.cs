namespace Dispatcher.Contracts.Realtime;

public static class RuntimeHubContract
{
    public const string Path = "/hubs/runtime";
    public const string TagChanged = "TagChanged";
    public const string DeviceStateChanged = "DeviceStateChanged";
    public const string ConfigurationChanged = "ConfigurationChanged";
}
