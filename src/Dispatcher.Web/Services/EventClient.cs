using System.Globalization;
using System.Net.Http.Json;
using System.Text.Json;
using Dispatcher.Contracts.Events;
using Dispatcher.Contracts.Realtime;
using Microsoft.AspNetCore.SignalR.Client;

namespace Dispatcher.Web.Services;

public sealed class EventClient : IAsyncDisposable
{
    private readonly HttpClient _httpClient;
    private readonly SemaphoreSlim _connectionLock =
        new(1, 1);

    private HubConnection? _hubConnection;
    private bool _initialized;

    public EventClient(
        HttpClient httpClient)
    {
        _httpClient =
            httpClient;
    }

    public event Action<EventRecordDto>? EventAdded;
    public event Action? ConnectionChanged;

    public EventRealtimeConnectionState ConnectionState { get; private set; } =
        EventRealtimeConnectionState.Disconnected;

    public string? LastError { get; private set; }

    public async Task<EventQueryResponseDto> QueryAsync(
        DateTimeOffset from,
        DateTimeOffset to,
        EventCategoryDto? category,
        EventSeverityDto? severity,
        string? source,
        string? text,
        int page,
        int limit,
        CancellationToken cancellationToken = default)
    {
        var query =
            new List<string>
            {
                $"from={Uri.EscapeDataString(from.ToUniversalTime().ToString("O", CultureInfo.InvariantCulture))}",
                $"to={Uri.EscapeDataString(to.ToUniversalTime().ToString("O", CultureInfo.InvariantCulture))}",
                $"page={page.ToString(CultureInfo.InvariantCulture)}",
                $"limit={limit.ToString(CultureInfo.InvariantCulture)}"
            };

        if (category is not null)
        {
            query.Add(
                $"category={Uri.EscapeDataString(category.Value.ToString())}");
        }

        if (severity is not null)
        {
            query.Add(
                $"severity={Uri.EscapeDataString(severity.Value.ToString())}");
        }

        if (!string.IsNullOrWhiteSpace(
                source))
        {
            query.Add(
                $"source={Uri.EscapeDataString(source.Trim())}");
        }

        if (!string.IsNullOrWhiteSpace(
                text))
        {
            query.Add(
                $"text={Uri.EscapeDataString(text.Trim())}");
        }

        using var response =
            await _httpClient.GetAsync(
                $"/api/events?{string.Join("&", query)}",
                cancellationToken);

        await EnsureSuccessAsync(
            response,
            cancellationToken);

        return await response.Content.ReadFromJsonAsync<EventQueryResponseDto>(
            cancellationToken:
                cancellationToken)
            ?? throw new InvalidOperationException(
                "Server returned an empty events response.");
    }

    public async Task StartRealtimeAsync()
    {
        if (!_initialized)
        {
            ConfigureHubConnection();
            _initialized = true;
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

        _hubConnection.On<EventRecordDto>(
            RuntimeHubContract.EventAdded,
            record =>
            {
                EventAdded?.Invoke(
                    record);
            });

        _hubConnection.Reconnecting +=
            exception =>
            {
                ConnectionState =
                    EventRealtimeConnectionState.Reconnecting;
                LastError =
                    exception?.Message;
                NotifyConnectionChanged();

                return Task.CompletedTask;
            };

        _hubConnection.Reconnected +=
            _ =>
            {
                ConnectionState =
                    EventRealtimeConnectionState.Connected;
                LastError =
                    null;
                NotifyConnectionChanged();

                return Task.CompletedTask;
            };

        _hubConnection.Closed +=
            exception =>
            {
                ConnectionState =
                    EventRealtimeConnectionState.Disconnected;
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
                EventRealtimeConnectionState.Connecting;
            NotifyConnectionChanged();

            try
            {
                await _hubConnection.StartAsync();

                ConnectionState =
                    EventRealtimeConnectionState.Connected;
                LastError =
                    null;
            }
            catch (Exception exception)
            {
                ConnectionState =
                    EventRealtimeConnectionState.Disconnected;
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
