using System.Globalization;
using System.Net.Http.Json;
using System.Text.Json;
using Dispatcher.Contracts.Alarms;
using Dispatcher.Contracts.Events;
using Dispatcher.Contracts.Realtime;
using Dispatcher.Contracts.Tags;
using Microsoft.AspNetCore.SignalR.Client;

namespace Dispatcher.Web.Services;

public sealed class AlarmRuntimeClient : IAsyncDisposable
{
    private readonly HttpClient _httpClient;
    private readonly SemaphoreSlim _connectionLock =
        new(1, 1);

    private HubConnection? _hubConnection;
    private bool _initialized;

    public AlarmRuntimeClient(
        HttpClient httpClient)
    {
        _httpClient =
            httpClient;
    }

    public event Action<AlarmRuntimeSnapshotDto>? AlarmChanged;
    public event Action<EventRecordDto>? AlarmEventAdded;
    public event Action<TagValueDto>? TagChanged;
    public event Action? ConnectionChanged;

    public AlarmRealtimeConnectionState ConnectionState { get; private set; } =
        AlarmRealtimeConnectionState.Disconnected;

    public string? LastError { get; private set; }

    public async Task<IReadOnlyList<AlarmRuntimeSnapshotDto>> GetCurrentAsync(
        CancellationToken cancellationToken = default)
    {
        using var response =
            await _httpClient.GetAsync(
                "/api/alarms/current",
                cancellationToken);

        await EnsureSuccessAsync(
            response,
            cancellationToken);

        return await response.Content.ReadFromJsonAsync<AlarmRuntimeSnapshotDto[]>(
                cancellationToken:
                    cancellationToken)
            ?? [];
    }

    public async Task<AlarmRuntimeSnapshotDto> AcknowledgeAsync(
        string alarmId,
        CancellationToken cancellationToken = default)
    {
        using var response =
            await _httpClient.PostAsync(
                $"/api/alarms/{Uri.EscapeDataString(alarmId)}/acknowledge",
                content:
                    null,
                cancellationToken);

        await EnsureSuccessAsync(
            response,
            cancellationToken);

        return await response.Content.ReadFromJsonAsync<AlarmRuntimeSnapshotDto>(
                cancellationToken:
                    cancellationToken)
            ?? throw new InvalidOperationException(
                "Server returned an empty alarm acknowledgement response.");
    }

    public async Task<AlarmHistoryQueryResponseDto> QueryHistoryAsync(
        DateTimeOffset from,
        DateTimeOffset to,
        int page,
        int limit,
        CancellationToken cancellationToken = default)
    {
        var query =
            string.Join(
                "&",
                $"from={Uri.EscapeDataString(from.ToUniversalTime().ToString("O", CultureInfo.InvariantCulture))}",
                $"to={Uri.EscapeDataString(to.ToUniversalTime().ToString("O", CultureInfo.InvariantCulture))}",
                $"page={page.ToString(CultureInfo.InvariantCulture)}",
                $"limit={limit.ToString(CultureInfo.InvariantCulture)}");

        using var response =
            await _httpClient.GetAsync(
                $"/api/alarms/history?{query}",
                cancellationToken);

        await EnsureSuccessAsync(
            response,
            cancellationToken);

        return await response.Content.ReadFromJsonAsync<AlarmHistoryQueryResponseDto>(
                cancellationToken:
                    cancellationToken)
            ?? throw new InvalidOperationException(
                "Server returned an empty alarm history response.");
    }

    public async Task StartRealtimeAsync()
    {
        if (!_initialized)
        {
            ConfigureHubConnection();
            _initialized =
                true;
        }

        if (_hubConnection?.State == HubConnectionState.Disconnected)
        {
            await TryStartHubAsync();
        }
    }

    private void ConfigureHubConnection()
    {
        var hubUri =
            new Uri(
                _httpClient.BaseAddress!,
                RuntimeHubContract.Path.TrimStart('/'));

        _hubConnection =
            new HubConnectionBuilder()
                .WithUrl(
                    hubUri)
                .WithAutomaticReconnect()
                .Build();

        _hubConnection.On<AlarmRuntimeSnapshotDto>(
            RuntimeHubContract.AlarmChanged,
            snapshot =>
            {
                AlarmChanged?.Invoke(
                    snapshot);
            });

        _hubConnection.On<EventRecordDto>(
            RuntimeHubContract.EventAdded,
            record =>
            {
                if (IsAlarmTransition(
                        record.Type))
                {
                    AlarmEventAdded?.Invoke(
                        record);
                }
            });

        _hubConnection.On<TagValueDto>(
            RuntimeHubContract.TagChanged,
            value =>
            {
                TagChanged?.Invoke(
                    value);
            });

        _hubConnection.Reconnecting +=
            exception =>
            {
                ConnectionState =
                    AlarmRealtimeConnectionState.Reconnecting;
                LastError =
                    exception?.Message;
                NotifyConnectionChanged();

                return Task.CompletedTask;
            };

        _hubConnection.Reconnected +=
            _ =>
            {
                ConnectionState =
                    AlarmRealtimeConnectionState.Connected;
                LastError =
                    null;
                NotifyConnectionChanged();

                return Task.CompletedTask;
            };

        _hubConnection.Closed +=
            exception =>
            {
                ConnectionState =
                    AlarmRealtimeConnectionState.Disconnected;
                LastError =
                    exception?.Message;
                NotifyConnectionChanged();

                return Task.CompletedTask;
            };
    }

    private async Task TryStartHubAsync()
    {
        if (_hubConnection is null)
        {
            return;
        }

        await _connectionLock.WaitAsync();

        try
        {
            if (_hubConnection.State != HubConnectionState.Disconnected)
            {
                return;
            }

            ConnectionState =
                AlarmRealtimeConnectionState.Connecting;
            NotifyConnectionChanged();

            try
            {
                await _hubConnection.StartAsync();

                ConnectionState =
                    AlarmRealtimeConnectionState.Connected;
                LastError =
                    null;
            }
            catch (Exception exception)
            {
                ConnectionState =
                    AlarmRealtimeConnectionState.Disconnected;
                LastError =
                    exception.Message;
            }

            NotifyConnectionChanged();
        }
        finally
        {
            _connectionLock.Release();
        }
    }

    private static bool IsAlarmTransition(
        string type)
    {
        return string.Equals(
                type,
                "AlarmRaised",
                StringComparison.Ordinal)
            || string.Equals(
                type,
                "AlarmAcknowledged",
                StringComparison.Ordinal)
            || string.Equals(
                type,
                "AlarmReturned",
                StringComparison.Ordinal);
    }

    private static async Task EnsureSuccessAsync(
        HttpResponseMessage response,
        CancellationToken cancellationToken)
    {
        if (response.IsSuccessStatusCode)
        {
            return;
        }

        var content =
            await response.Content.ReadAsStringAsync(
                cancellationToken);

        if (!string.IsNullOrWhiteSpace(
                content))
        {
            try
            {
                using var document =
                    JsonDocument.Parse(
                        content);

                var root =
                    document.RootElement;

                if (root.TryGetProperty(
                        "detail",
                        out var detail)
                    && detail.ValueKind == JsonValueKind.String)
                {
                    throw new InvalidOperationException(
                        detail.GetString()
                        ?? content);
                }

                if (root.TryGetProperty(
                        "title",
                        out var title)
                    && title.ValueKind == JsonValueKind.String)
                {
                    throw new InvalidOperationException(
                        title.GetString()
                        ?? content);
                }
            }
            catch (JsonException)
            {
            }

            throw new InvalidOperationException(
                content);
        }

        throw new InvalidOperationException(
            $"HTTP {(int)response.StatusCode} {response.ReasonPhrase}");
    }

    private void NotifyConnectionChanged()
    {
        ConnectionChanged?.Invoke();
    }

    public async ValueTask DisposeAsync()
    {
        if (_hubConnection is not null)
        {
            await _hubConnection.DisposeAsync();
        }

        _connectionLock.Dispose();
    }
}
