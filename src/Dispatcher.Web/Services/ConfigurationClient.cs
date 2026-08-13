using System.Net.Http.Json;
using System.Text.Json;
using Dispatcher.Contracts.Configuration;

namespace Dispatcher.Web.Services;

public sealed class ConfigurationClient
{
    private readonly HttpClient _httpClient;

    public ConfigurationClient(
        HttpClient httpClient)
    {
        _httpClient = httpClient;
    }

    public async Task<IReadOnlyList<ModbusDeviceConfigurationDto>> GetDevicesAsync(
        CancellationToken cancellationToken = default)
    {
        return await _httpClient.GetFromJsonAsync<
            ModbusDeviceConfigurationDto[]>(
                "/api/configuration/modbus/devices",
                cancellationToken)
            ?? [];
    }

    public async Task<ModbusDeviceConfigurationDto> CreateDeviceAsync(
        ModbusDeviceUpsertRequest request,
        CancellationToken cancellationToken = default)
    {
        using var response =
            await _httpClient.PostAsJsonAsync(
                "/api/configuration/modbus/devices",
                request,
                cancellationToken);

        return await ReadRequiredAsync<
            ModbusDeviceConfigurationDto>(
                response,
                cancellationToken);
    }

    public async Task<ModbusDeviceConfigurationDto> UpdateDeviceAsync(
        string deviceId,
        ModbusDeviceUpsertRequest request,
        CancellationToken cancellationToken = default)
    {
        using var response =
            await _httpClient.PutAsJsonAsync(
                $"/api/configuration/modbus/devices/{Escape(deviceId)}",
                request,
                cancellationToken);

        return await ReadRequiredAsync<
            ModbusDeviceConfigurationDto>(
                response,
                cancellationToken);
    }

    public async Task DeleteDeviceAsync(
        string deviceId,
        CancellationToken cancellationToken = default)
    {
        using var response =
            await _httpClient.DeleteAsync(
                $"/api/configuration/modbus/devices/{Escape(deviceId)}",
                cancellationToken);

        await EnsureSuccessAsync(
            response,
            cancellationToken);
    }

    public async Task<ModbusTagConfigurationDto> CreateTagAsync(
        string deviceId,
        ModbusTagUpsertRequest request,
        CancellationToken cancellationToken = default)
    {
        using var response =
            await _httpClient.PostAsJsonAsync(
                $"/api/configuration/modbus/devices/{Escape(deviceId)}/tags",
                request,
                cancellationToken);

        return await ReadRequiredAsync<
            ModbusTagConfigurationDto>(
                response,
                cancellationToken);
    }

    public async Task<ModbusTagConfigurationDto> UpdateTagAsync(
        string deviceId,
        string tagId,
        ModbusTagUpsertRequest request,
        CancellationToken cancellationToken = default)
    {
        using var response =
            await _httpClient.PutAsJsonAsync(
                $"/api/configuration/modbus/devices/{Escape(deviceId)}/tags/{Escape(tagId)}",
                request,
                cancellationToken);

        return await ReadRequiredAsync<
            ModbusTagConfigurationDto>(
                response,
                cancellationToken);
    }

    public async Task DeleteTagAsync(
        string deviceId,
        string tagId,
        CancellationToken cancellationToken = default)
    {
        using var response =
            await _httpClient.DeleteAsync(
                $"/api/configuration/modbus/devices/{Escape(deviceId)}/tags/{Escape(tagId)}",
                cancellationToken);

        await EnsureSuccessAsync(
            response,
            cancellationToken);
    }

    private static string Escape(
        string value)
    {
        return Uri.EscapeDataString(value);
    }

    private static async Task<T> ReadRequiredAsync<T>(
        HttpResponseMessage response,
        CancellationToken cancellationToken)
    {
        await EnsureSuccessAsync(
            response,
            cancellationToken);

        var result =
            await response.Content.ReadFromJsonAsync<T>(
                cancellationToken: cancellationToken);

        return result
            ?? throw new InvalidOperationException(
                "Server returned an empty configuration response.");
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

        if (!string.IsNullOrWhiteSpace(content))
        {
            try
            {
                using var document =
                    JsonDocument.Parse(content);

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
}
