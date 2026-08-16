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
    public const string ConfigurationChanged = "ConfigurationChanged";

    public const string LoginSucceeded = "LoginSucceeded";
    public const string LoginFailed = "LoginFailed";

    public const string SecurityUserCreated = "SecurityUserCreated";
    public const string SecurityUserUpdated = "SecurityUserUpdated";
    public const string SecurityUserPasswordReset = "SecurityUserPasswordReset";
    public const string SecurityUserRolesChanged = "SecurityUserRolesChanged";

    public const string SecurityRoleCreated = "SecurityRoleCreated";
    public const string SecurityRoleUpdated = "SecurityRoleUpdated";
    public const string SecurityRoleDeleted = "SecurityRoleDeleted";
}
