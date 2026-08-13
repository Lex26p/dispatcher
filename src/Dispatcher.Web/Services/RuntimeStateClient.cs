using System.Net.Http.Json;
using System.Text.Json;
using Dispatcher.Contracts.Devices;
using Dispatcher.Contracts.Realtime;
using Dispatcher.Contracts.Tags;
using Microsoft.AspNetCore.SignalR.Client;

namespace Dispatcher.Web.Services;

public sealed class RuntimeStateClient : IAsyncDisposable
{
    private readonly HttpClient _httpClient;
    private readonly Dictionary<string, TagValueDto> _tags =
        new(StringComparer.Ordinal);
    private readonly Dictionary<string, DeviceStateDto> _devices =
        new(StringComparer.Ordinal);
    private readonly SemaphoreSlim _connectionLock = new(1, 1);

    private HubConnection? _hubConnection;
    private bool _initialized;

    public RuntimeStateClient(HttpClient httpClient)
    {
        _httpClient = httpClient;
    }

    public event Action? Changed;

    public IReadOnlyList<TagValueDto> Tags =>
        _tags.Values
            .OrderBy(tag => tag.TagId, StringComparer.Ordinal)
            .ToArray();

    public IReadOnlyList<DeviceStateDto> Devices =>
        _devices.Values
            .OrderBy(device => device.DeviceId, StringComparer.Ordinal)
            .ToArray();

    public RuntimeConnectionState ConnectionState { get; private set; } =
        RuntimeConnectionState.Disconnected;

    public string? LastError { get; private set; }

    public async Task StartAsync()
    {
        if (!_initialized)
        {
            ConfigureHubConnection();
            _initialized = true;
        }

        await RefreshAsync();
    }

    public async Task RefreshAsync()
    {
        await ReloadSnapshotAsync();

        if (_hubConnection?.State == HubConnectionState.Disconnected)
        {
            await TryStartHubAsync();
        }
    }

    public async Task<string?> WriteTagAsync(
        string tagId,
        ushort value)
    {
        try
        {
            using var response = await _httpClient.PostAsJsonAsync(
                $"/api/tags/{Uri.EscapeDataString(tagId)}/write",
                new TagWriteRequest(value));

            if (!response.IsSuccessStatusCode)
            {
                return await ReadProblemAsync(response);
            }

            var updated = await response.Content
                .ReadFromJsonAsync<TagValueDto>();

            if (updated is not null)
            {
                _tags[updated.TagId] = updated;
                NotifyChanged();
            }

            return null;
        }
        catch (Exception exception)
        {
            return exception.Message;
        }
    }

    private void ConfigureHubConnection()
    {
        var hubUri = new Uri(
            _httpClient.BaseAddress!,
            RuntimeHubContract.Path.TrimStart('/'));

        _hubConnection = new HubConnectionBuilder()
            .WithUrl(hubUri)
            .WithAutomaticReconnect()
            .Build();

        _hubConnection.On<TagValueDto>(
            RuntimeHubContract.TagChanged,
            OnTagChanged);

        _hubConnection.On<DeviceStateDto>(
            RuntimeHubContract.DeviceStateChanged,
            OnDeviceStateChanged);

        _hubConnection.Reconnecting += exception =>
        {
            ConnectionState = RuntimeConnectionState.Reconnecting;
            LastError = exception?.Message;
            NotifyChanged();

            return Task.CompletedTask;
        };

        _hubConnection.Reconnected += async _ =>
        {
            ConnectionState = RuntimeConnectionState.Connected;
            LastError = null;

            await ReloadSnapshotAsync();
            NotifyChanged();
        };

        _hubConnection.Closed += exception =>
        {
            ConnectionState = RuntimeConnectionState.Disconnected;
            LastError = exception?.Message;
            NotifyChanged();

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

            ConnectionState = RuntimeConnectionState.Connecting;
            NotifyChanged();

            try
            {
                await _hubConnection.StartAsync();

                ConnectionState = RuntimeConnectionState.Connected;
                LastError = null;
            }
            catch (Exception exception)
            {
                ConnectionState = RuntimeConnectionState.Disconnected;
                LastError = exception.Message;
            }

            NotifyChanged();
        }
        finally
        {
            _connectionLock.Release();
        }
    }

    private async Task ReloadSnapshotAsync()
    {
        try
        {
            var tags = await _httpClient.GetFromJsonAsync<TagValueDto[]>(
                "/api/tags") ?? [];

            var devices = await _httpClient.GetFromJsonAsync<DeviceStateDto[]>(
                "/api/devices") ?? [];

            _tags.Clear();

            foreach (var tag in tags)
            {
                _tags[tag.TagId] = tag;
            }

            _devices.Clear();

            foreach (var device in devices)
            {
                _devices[device.DeviceId] = device;
            }

            LastError = null;
        }
        catch (Exception exception)
        {
            LastError = exception.Message;
        }

        NotifyChanged();
    }

    private static async Task<string> ReadProblemAsync(
        HttpResponseMessage response)
    {
        var content = await response.Content.ReadAsStringAsync();

        if (!string.IsNullOrWhiteSpace(content))
        {
            try
            {
                using var document = JsonDocument.Parse(content);
                var root = document.RootElement;

                if (root.TryGetProperty(
                        "detail",
                        out var detail)
                    && detail.ValueKind == JsonValueKind.String)
                {
                    return detail.GetString() ?? content;
                }

                if (root.TryGetProperty(
                        "title",
                        out var title)
                    && title.ValueKind == JsonValueKind.String)
                {
                    return title.GetString() ?? content;
                }
            }
            catch (JsonException)
            {
            }

            return content;
        }

        return $"HTTP {(int)response.StatusCode} {response.ReasonPhrase}";
    }

    private void OnTagChanged(TagValueDto tag)
    {
        _tags[tag.TagId] = tag;
        NotifyChanged();
    }

    private void OnDeviceStateChanged(DeviceStateDto state)
    {
        _devices[state.DeviceId] = state;
        NotifyChanged();
    }

    private void NotifyChanged()
    {
        Changed?.Invoke();
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
