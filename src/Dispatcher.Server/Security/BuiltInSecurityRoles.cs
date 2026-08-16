using Dispatcher.Contracts.Authorization;

namespace Dispatcher.Server.Security;

public static class BuiltInSecurityRoles
{
    public const string ViewerRoleId = "viewer";
    public const string OperatorRoleId = "operator";
    public const string EngineerRoleId = "engineer";
    public const string AdministratorRoleId = "administrator";

    public static IReadOnlyList<SecurityRoleConfiguration> All { get; } =
    [
        Create(
            ViewerRoleId,
            "Viewer",
            [
                PermissionNames.RuntimeRead
            ]),
        Create(
            OperatorRoleId,
            "Operator",
            [
                PermissionNames.RuntimeRead,
                PermissionNames.TagsWrite,
                PermissionNames.AlarmsAcknowledge
            ]),
        Create(
            EngineerRoleId,
            "Engineer",
            [
                PermissionNames.RuntimeRead,
                PermissionNames.TagsWrite,
                PermissionNames.DevicesEdit,
                PermissionNames.MimicsEdit,
                PermissionNames.HistorianConfigure,
                PermissionNames.AlarmsConfigure,
                PermissionNames.AlarmsAcknowledge,
                PermissionNames.TemplatesEdit,
                PermissionNames.ScriptsEdit,
                PermissionNames.ScriptsExecute
            ]),
        Create(
            AdministratorRoleId,
            "Administrator",
            PermissionNames.All)
    ];

    private static SecurityRoleConfiguration Create(
        string roleId,
        string name,
        IReadOnlyList<string> permissions)
    {
        return new SecurityRoleConfiguration(
            RoleId:
                roleId,
            Name:
                name,
            NormalizedName:
                SecurityRoleConfiguration.NormalizeName(
                    name),
            BuiltIn:
                true,
            Permissions:
                permissions.ToArray());
    }
}
