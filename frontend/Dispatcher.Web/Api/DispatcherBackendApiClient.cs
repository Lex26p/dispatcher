using System.Net.Http.Json;
using Dispatcher.Web.Api.Models;
using Dispatcher.Web.Services;

namespace Dispatcher.Web.Api;

public sealed class DispatcherBackendApiClient
{
    private readonly DispatcherBackendHttpClient _backend;

    public DispatcherBackendApiClient(
        DispatcherBackendHttpClient backend
    )
    {
        _backend = backend;
    }

    public Task<ApiCallResult<HealthResponseDto>> GetHealthAsync(
        CancellationToken cancellationToken = default
    )
    {
        return GetJsonAsync<HealthResponseDto>(
            _backend.Options.HealthEndpoint,
            cancellationToken
        );
    }

    public Task<ApiCallResult<ReadinessResponseDto>> GetReadinessAsync(
        CancellationToken cancellationToken = default
    )
    {
        return GetJsonAsync<ReadinessResponseDto>(
            _backend.Options.ReadinessEndpoint,
            cancellationToken
        );
    }

    public Task<ApiCallResult<RuntimeResponseDto>> GetRuntimeAsync(
        CancellationToken cancellationToken = default
    )
    {
        return GetJsonAsync<RuntimeResponseDto>(
            _backend.Options.RuntimeEndpoint,
            cancellationToken
        );
    }

    public Task<ApiCallResult<AlarmsResponseDto>> GetAlarmsAsync(
        CancellationToken cancellationToken = default
    )
    {
        return GetJsonAsync<AlarmsResponseDto>(
            _backend.Options.AlarmsEndpoint,
            cancellationToken
        );
    }

    private async Task<ApiCallResult<T>> GetJsonAsync<T>(
        string endpoint,
        CancellationToken cancellationToken
    )
    {
        try
        {
            var response =
                await _backend.Client.GetAsync(
                    endpoint,
                    cancellationToken
                );

            if (!response.IsSuccessStatusCode)
            {
                return ApiCallResult<T>.Fail(
                    $"Backend returned HTTP {(int)response.StatusCode}."
                );
            }

            var value =
                await response.Content.ReadFromJsonAsync<T>(
                    cancellationToken
                );

            if (value is null)
            {
                return ApiCallResult<T>.Fail(
                    "Backend returned an empty response."
                );
            }

            return ApiCallResult<T>.Ok(
                value
            );
        }
        catch (OperationCanceledException)
        {
            return ApiCallResult<T>.Fail(
                "Backend request was cancelled or timed out."
            );
        }
        catch (HttpRequestException exception)
        {
            return ApiCallResult<T>.Fail(
                exception.Message
            );
        }
        catch (NotSupportedException exception)
        {
            return ApiCallResult<T>.Fail(
                exception.Message
            );
        }
        catch (System.Text.Json.JsonException exception)
        {
            return ApiCallResult<T>.Fail(
                exception.Message
            );
        }
    }
}