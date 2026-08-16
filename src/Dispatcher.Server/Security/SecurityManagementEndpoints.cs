using Dispatcher.Contracts.Security;
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
        SecurityManagementService service,
        CancellationToken cancellationToken)
    {
        try
        {
            var user =
                await service.CreateUserAsync(
                    request,
                    cancellationToken);

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
        SecurityManagementService service,
        CancellationToken cancellationToken)
    {
        try
        {
            return Results.Ok(
                await service.UpdateUserAsync(
                    userId,
                    request,
                    cancellationToken));
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
        SecurityManagementService service,
        CancellationToken cancellationToken)
    {
        try
        {
            await service.ResetUserPasswordAsync(
                userId,
                request,
                cancellationToken);

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
        SecurityManagementService service,
        CancellationToken cancellationToken)
    {
        try
        {
            return Results.Ok(
                await service.ReplaceUserRolesAsync(
                    userId,
                    request,
                    cancellationToken));
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
        SecurityManagementService service,
        CancellationToken cancellationToken)
    {
        try
        {
            var role =
                await service.CreateRoleAsync(
                    request,
                    cancellationToken);

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
        SecurityManagementService service,
        CancellationToken cancellationToken)
    {
        try
        {
            return Results.Ok(
                await service.UpdateRoleAsync(
                    roleId,
                    request,
                    cancellationToken));
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
        SecurityManagementService service,
        CancellationToken cancellationToken)
    {
        try
        {
            await service.DeleteRoleAsync(
                roleId,
                cancellationToken);

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
