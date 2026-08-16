using Dispatcher.Contracts.Security;
using Dispatcher.Server.Events;
using Microsoft.AspNetCore.Builder;
using Microsoft.AspNetCore.Http;
using Microsoft.AspNetCore.Routing;

namespace Dispatcher.Server.Security;

public static class SecurityManagementEndpoints
{
    public static IEndpointRouteBuilder MapSecurityManagementEndpoints(
        this IEndpointRouteBuilder endpoints)
    {
        var group =
            endpoints.MapGroup(
                "/api/security");

        group.MapGet(
            "/users",
            GetUsersAsync);
        group.MapPost(
            "/users",
            CreateUserAsync);
        group.MapPut(
            "/users/{userId}",
            UpdateUserAsync);
        group.MapPut(
            "/users/{userId}/password",
            ResetUserPasswordAsync);
        group.MapPut(
            "/users/{userId}/roles",
            ReplaceUserRolesAsync);

        group.MapGet(
            "/roles",
            GetRolesAsync);
        group.MapPost(
            "/roles",
            CreateRoleAsync);
        group.MapPut(
            "/roles/{roleId}",
            UpdateRoleAsync);
        group.MapDelete(
            "/roles/{roleId}",
            DeleteRoleAsync);

        return endpoints;
    }

    private static async Task<IResult> GetUsersAsync(
        HttpContext httpContext,
        SecurityManagementService service,
        CancellationToken cancellationToken)
    {
        SetNoStore(
            httpContext.Response);

        return Results.Ok(
            await service.GetUsersAsync(
                cancellationToken));
    }

    private static async Task<IResult> CreateUserAsync(
        CreateSecurityUserRequest request,
        HttpContext httpContext,
        SecurityManagementService service,
        EventJournalService eventJournal,
        CancellationToken cancellationToken)
    {
        try
        {
            var actor =
                EventActor.FromAuthenticatedPrincipal(
                    httpContext.User);
            var user =
                await service.CreateUserAsync(
                    request,
                    cancellationToken);

            PublishSecurityAudit(
                eventJournal,
                actor,
                EventTypes.SecurityUserCreated,
                $"Создан пользователь '{user.UserName}'.",
                new
                {
                    user.UserId,
                    user.UserName,
                    user.DisplayName,
                    user.Enabled
                });

            return Results.Created(
                $"/api/security/users/{Uri.EscapeDataString(user.UserId)}",
                user);
        }
        catch (OperationCanceledException)
            when (cancellationToken.IsCancellationRequested)
        {
            throw;
        }
        catch (Exception exception)
        {
            return ToProblem(
                exception);
        }
    }

    private static async Task<IResult> UpdateUserAsync(
        string userId,
        UpdateSecurityUserRequest request,
        HttpContext httpContext,
        SecurityManagementService service,
        EventJournalService eventJournal,
        CancellationToken cancellationToken)
    {
        try
        {
            var actor =
                EventActor.FromAuthenticatedPrincipal(
                    httpContext.User);
            var user =
                await service.UpdateUserAsync(
                    userId,
                    request,
                    cancellationToken);

            PublishSecurityAudit(
                eventJournal,
                actor,
                EventTypes.SecurityUserUpdated,
                $"Изменён пользователь '{user.UserName}'.",
                new
                {
                    user.UserId,
                    user.UserName,
                    user.DisplayName,
                    user.Enabled
                });

            return Results.Ok(
                user);
        }
        catch (OperationCanceledException)
            when (cancellationToken.IsCancellationRequested)
        {
            throw;
        }
        catch (Exception exception)
        {
            return ToProblem(
                exception);
        }
    }

    private static async Task<IResult> ResetUserPasswordAsync(
        string userId,
        ResetSecurityUserPasswordRequest request,
        HttpContext httpContext,
        SecurityManagementService service,
        EventJournalService eventJournal,
        CancellationToken cancellationToken)
    {
        try
        {
            var actor =
                EventActor.FromAuthenticatedPrincipal(
                    httpContext.User);

            await service.ResetUserPasswordAsync(
                userId,
                request,
                cancellationToken);

            PublishSecurityAudit(
                eventJournal,
                actor,
                EventTypes.SecurityUserPasswordReset,
                $"Сброшен пароль пользователя '{userId}'.",
                new
                {
                    UserId =
                        userId
                });

            return Results.NoContent();
        }
        catch (OperationCanceledException)
            when (cancellationToken.IsCancellationRequested)
        {
            throw;
        }
        catch (Exception exception)
        {
            return ToProblem(
                exception);
        }
    }

    private static async Task<IResult> ReplaceUserRolesAsync(
        string userId,
        ReplaceSecurityUserRolesRequest request,
        HttpContext httpContext,
        SecurityManagementService service,
        EventJournalService eventJournal,
        CancellationToken cancellationToken)
    {
        try
        {
            var actor =
                EventActor.FromAuthenticatedPrincipal(
                    httpContext.User);
            var user =
                await service.ReplaceUserRolesAsync(
                    userId,
                    request,
                    cancellationToken);

            PublishSecurityAudit(
                eventJournal,
                actor,
                EventTypes.SecurityUserRolesChanged,
                $"Изменены роли пользователя '{user.UserName}'.",
                new
                {
                    user.UserId,
                    user.UserName,
                    user.RoleIds,
                    user.EffectivePermissions
                });

            return Results.Ok(
                user);
        }
        catch (OperationCanceledException)
            when (cancellationToken.IsCancellationRequested)
        {
            throw;
        }
        catch (Exception exception)
        {
            return ToProblem(
                exception);
        }
    }

    private static async Task<IResult> GetRolesAsync(
        HttpContext httpContext,
        SecurityManagementService service,
        CancellationToken cancellationToken)
    {
        SetNoStore(
            httpContext.Response);

        return Results.Ok(
            await service.GetRolesAsync(
                cancellationToken));
    }

    private static async Task<IResult> CreateRoleAsync(
        SecurityRoleUpsertRequest request,
        HttpContext httpContext,
        SecurityManagementService service,
        EventJournalService eventJournal,
        CancellationToken cancellationToken)
    {
        try
        {
            var actor =
                EventActor.FromAuthenticatedPrincipal(
                    httpContext.User);
            var role =
                await service.CreateRoleAsync(
                    request,
                    cancellationToken);

            PublishSecurityAudit(
                eventJournal,
                actor,
                EventTypes.SecurityRoleCreated,
                $"Создана роль '{role.Name}'.",
                new
                {
                    role.RoleId,
                    role.Name,
                    role.Permissions
                });

            return Results.Created(
                $"/api/security/roles/{Uri.EscapeDataString(role.RoleId)}",
                role);
        }
        catch (OperationCanceledException)
            when (cancellationToken.IsCancellationRequested)
        {
            throw;
        }
        catch (Exception exception)
        {
            return ToProblem(
                exception);
        }
    }

    private static async Task<IResult> UpdateRoleAsync(
        string roleId,
        SecurityRoleUpsertRequest request,
        HttpContext httpContext,
        SecurityManagementService service,
        EventJournalService eventJournal,
        CancellationToken cancellationToken)
    {
        try
        {
            var actor =
                EventActor.FromAuthenticatedPrincipal(
                    httpContext.User);
            var role =
                await service.UpdateRoleAsync(
                    roleId,
                    request,
                    cancellationToken);

            PublishSecurityAudit(
                eventJournal,
                actor,
                EventTypes.SecurityRoleUpdated,
                $"Изменена роль '{role.Name}'.",
                new
                {
                    role.RoleId,
                    role.Name,
                    role.Permissions
                });

            return Results.Ok(
                role);
        }
        catch (OperationCanceledException)
            when (cancellationToken.IsCancellationRequested)
        {
            throw;
        }
        catch (Exception exception)
        {
            return ToProblem(
                exception);
        }
    }

    private static async Task<IResult> DeleteRoleAsync(
        string roleId,
        HttpContext httpContext,
        SecurityManagementService service,
        EventJournalService eventJournal,
        CancellationToken cancellationToken)
    {
        try
        {
            var actor =
                EventActor.FromAuthenticatedPrincipal(
                    httpContext.User);

            await service.DeleteRoleAsync(
                roleId,
                cancellationToken);

            PublishSecurityAudit(
                eventJournal,
                actor,
                EventTypes.SecurityRoleDeleted,
                $"Удалена роль '{roleId}'.",
                new
                {
                    RoleId =
                        roleId
                });

            return Results.NoContent();
        }
        catch (OperationCanceledException)
            when (cancellationToken.IsCancellationRequested)
        {
            throw;
        }
        catch (Exception exception)
        {
            return ToProblem(
                exception);
        }
    }

    private static void PublishSecurityAudit(
        EventJournalService eventJournal,
        EventActor actor,
        string type,
        string message,
        object? data)
    {
        eventJournal.Publish(
            EventCategory.Configuration,
            type,
            EventSeverity.Information,
            source:
                "security",
            message:
                message,
            data:
                data,
            actor:
                actor);
    }

    private static void SetNoStore(
        HttpResponse response)
    {
        response.Headers[
            "Cache-Control"] =
            "no-store";
        response.Headers[
            "Pragma"] =
            "no-cache";
    }

    private static IResult ToProblem(
        Exception exception)
    {
        return exception switch
        {
            SecurityManagementNotFoundException =>
                Results.Problem(
                    statusCode:
                        StatusCodes.Status404NotFound,
                    title:
                        "Security object not found.",
                    detail:
                        exception.Message),

            SecurityManagementConflictException =>
                Results.Problem(
                    statusCode:
                        StatusCodes.Status409Conflict,
                    title:
                        "Security configuration conflict.",
                    detail:
                        exception.Message),

            ArgumentException or InvalidOperationException =>
                Results.Problem(
                    statusCode:
                        StatusCodes.Status400BadRequest,
                    title:
                        "Invalid security configuration.",
                    detail:
                        exception.Message),

            _ =>
                Results.Problem(
                    statusCode:
                        StatusCodes.Status500InternalServerError,
                    title:
                        "Security management operation failed.",
                    detail:
                        exception.Message)
        };
    }
}
