namespace Dispatcher.Contracts.Authorization;

public static class PermissionNames
{
    public const string RuntimeRead = "Runtime.Read";
    public const string TagsWrite = "Tags.Write";
    public const string DevicesEdit = "Devices.Edit";
    public const string MimicsEdit = "Mimics.Edit";
    public const string HistorianConfigure = "Historian.Configure";
    public const string AlarmsConfigure = "Alarms.Configure";
    public const string AlarmsAcknowledge = "Alarms.Acknowledge";
    public const string UsersManage = "Users.Manage";
    public const string RolesManage = "Roles.Manage";
    public const string TemplatesEdit = "Templates.Edit";
    public const string ScriptsEdit = "Scripts.Edit";
    public const string ScriptsExecute = "Scripts.Execute";

    public static IReadOnlyList<string> All { get; } =
    [
        RuntimeRead,
        TagsWrite,
        DevicesEdit,
        MimicsEdit,
        HistorianConfigure,
        AlarmsConfigure,
        AlarmsAcknowledge,
        UsersManage,
        RolesManage,
        TemplatesEdit,
        ScriptsEdit,
        ScriptsExecute
    ];

    private static readonly HashSet<string> Known =
        new(
            All,
            StringComparer.Ordinal);

    public static bool IsKnown(string permission)
    {
        return Known.Contains(permission);
    }
}
