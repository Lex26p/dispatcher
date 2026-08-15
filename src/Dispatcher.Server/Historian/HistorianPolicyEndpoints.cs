using Dispatcher.Contracts.Historian;
using Dispatcher.Server.Configuration;

namespace Dispatcher.Server.Historian;

public static class HistorianPolicyEndpoints
{
    public static IEndpointRouteBuilder MapHistorianPolicyEndpoints(
        this IEndpointRouteBuilder endpoints)
    {
        var group =
            endpoints.MapGroup(
                "/api/configuration/historian");

        group.MapGet(
            "/policies",
            (
                HistorianPolicyService service,
                ConfigurationCatalog configuration) =>
            {
                return service.GetAll()
                    .Select(policy =>
                        HistorianContractMapper.ToDto(
                            policy,
                            configuration))
                    .ToArray();
            });

        group.MapPut(
            "/policies/{tagId}",
            UpsertPolicyAsync);

        group.MapDelete(
            "/policies/{tagId}",
            DeletePolicyAsync);

        return endpoints;
    }

    private static async Task<IResult> UpsertPolicyAsync(
        string tagId,
        HistorianPolicyUpsertRequest request,
        HistorianPolicyService service,
        ConfigurationCatalog configuration,
        CancellationToken cancellationToken)
    {
        try
        {
            var policy =
                await service.UpsertAsync(
                    tagId,
                    request,
                    cancellationToken);

            return Results.Ok(
                HistorianContractMapper.ToDto(
                    policy,
                    configuration));
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

    private static async Task<IResult> DeletePolicyAsync(
        string tagId,
        HistorianPolicyService service,
        CancellationToken cancellationToken)
    {
        try
        {
            await service.DeleteAsync(
                tagId,
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

    private static IResult ToProblem(
        Exception exception)
    {
        return exception switch
        {
            KeyNotFoundException =>
                Results.Problem(
                    statusCode:
                        StatusCodes.Status404NotFound,
                    title:
                        "Historian policy target not found.",
                    detail:
                        exception.Message),

            InvalidOperationException or
            ArgumentException =>
                Results.Problem(
                    statusCode:
                        StatusCodes.Status400BadRequest,
                    title:
                        "Invalid historian policy.",
                    detail:
                        exception.Message),

            _ =>
                Results.Problem(
                    statusCode:
                        StatusCodes.Status500InternalServerError,
                    title:
                        "Historian policy update failed.",
                    detail:
                        exception.Message)
        };
    }
}
