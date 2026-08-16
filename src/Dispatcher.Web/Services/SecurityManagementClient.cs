using System.Net;
using System.Net.Http.Json;
using System.Text.Json;
using Dispatcher.Contracts.Security;

namespace Dispatcher.Web.Services;

public sealed class SecurityManagementClient
{
    private readonly HttpClient _httpClient;

    public SecurityManagementClient(
        HttpClient httpClient)
    {
        _httpClient =
            httpClient;
    }

    public Task<IReadOnlyList<SecurityUserDto>> GetUsersAsync(
        CancellationToken cancellationToken = default)
    {
        return GetArrayAsync<SecurityUserDto>(
            "/api/security/users",
            cancellationToken);
    }

    public Task<IReadOnlyList<SecurityRoleDto>> GetRolesAsync(
        CancellationToken cancellationToken = default)
    {
        return GetArrayAsync<SecurityRoleDto>(
            "/api/security/roles",
            cancellationToken);
    }

    public async Task<SecurityUserDto> CreateUserAsync(
        CreateSecurityUserRequest request,
        CancellationToken cancellationToken = default)
    {
        using var response =
            await _httpClient.PostAsJsonAsync(
                "/api/security/users",
                request,
                cancellationToken);

        return await ReadRequiredAsync<SecurityUserDto>(
            response,
            cancellationToken);
    }

    public async Task<SecurityUserDto> UpdateUserAsync(
        string userId,
        UpdateSecurityUserRequest request,
        CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            userId);

        using var response =
            await _httpClient.PutAsJsonAsync(
                $"/api/security/users/{Uri.EscapeDataString(userId)}",
                request,
                cancellationToken);

        return await ReadRequiredAsync<SecurityUserDto>(
            response,
            cancellationToken);
    }

    public async Task ResetUserPasswordAsync(
        string userId,
        ResetSecurityUserPasswordRequest request,
        CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            userId);

        using var response =
            await _httpClient.PutAsJsonAsync(
                $"/api/security/users/{Uri.EscapeDataString(userId)}/password",
                request,
                cancellationToken);

        await EnsureSuccessAsync(
            response,
            cancellationToken);
    }

    public async Task<SecurityUserDto> ReplaceUserRolesAsync(
        string userId,
        ReplaceSecurityUserRolesRequest request,
        CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            userId);

        using var response =
            await _httpClient.PutAsJsonAsync(
                $"/api/security/users/{Uri.EscapeDataString(userId)}/roles",
                request,
                cancellationToken);

        return await ReadRequiredAsync<SecurityUserDto>(
            response,
            cancellationToken);
    }

    public async Task<SecurityRoleDto> CreateRoleAsync(
        SecurityRoleUpsertRequest request,
        CancellationToken cancellationToken = default)
    {
        using var response =
            await _httpClient.PostAsJsonAsync(
                "/api/security/roles",
                request,
                cancellationToken);

        return await ReadRequiredAsync<SecurityRoleDto>(
            response,
            cancellationToken);
    }

    public async Task<SecurityRoleDto> UpdateRoleAsync(
        string roleId,
        SecurityRoleUpsertRequest request,
        CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            roleId);

        using var response =
            await _httpClient.PutAsJsonAsync(
                $"/api/security/roles/{Uri.EscapeDataString(roleId)}",
                request,
                cancellationToken);

        return await ReadRequiredAsync<SecurityRoleDto>(
            response,
            cancellationToken);
    }

    public async Task DeleteRoleAsync(
        string roleId,
        CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            roleId);

        using var response =
            await _httpClient.DeleteAsync(
                $"/api/security/roles/{Uri.EscapeDataString(roleId)}",
                cancellationToken);

        await EnsureSuccessAsync(
            response,
            cancellationToken);
    }

    private async Task<IReadOnlyList<T>> GetArrayAsync<T>(
        string uri,
        CancellationToken cancellationToken)
    {
        using var response =
            await _httpClient.GetAsync(
                uri,
                cancellationToken);

        await EnsureSuccessAsync(
            response,
            cancellationToken);

        var items =
            await response.Content.ReadFromJsonAsync<T[]>(
                cancellationToken:
                    cancellationToken);

        return items
            ?? throw new InvalidOperationException(
                $"Security API returned an empty response for '{uri}'.");
    }

    private static async Task<T> ReadRequiredAsync<T>(
        HttpResponseMessage response,
        CancellationToken cancellationToken)
    {
        await EnsureSuccessAsync(
            response,
            cancellationToken);

        var value =
            await response.Content.ReadFromJsonAsync<T>(
                cancellationToken:
                    cancellationToken);

        return value
            ?? throw new InvalidOperationException(
                "Security API returned an empty response.");
    }

    private static async Task EnsureSuccessAsync(
        HttpResponseMessage response,
        CancellationToken cancellationToken)
    {
        if (response.IsSuccessStatusCode)
        {
            return;
        }

        throw new SecurityManagementClientException(
            response.StatusCode,
            await ReadProblemMessageAsync(
                response,
                cancellationToken));
    }

    private static async Task<string> ReadProblemMessageAsync(
        HttpResponseMessage response,
        CancellationToken cancellationToken)
    {
        try
        {
            var text =
                await response.Content.ReadAsStringAsync(
                    cancellationToken);

            if (!string.IsNullOrWhiteSpace(
                    text))
            {
                using var document =
                    JsonDocument.Parse(
                        text);

                if (document.RootElement.ValueKind
                    == JsonValueKind.Object)
                {
                    if (document.RootElement.TryGetProperty(
                            "detail",
                            out var detail)
                        && detail.ValueKind
                            == JsonValueKind.String
                        && !string.IsNullOrWhiteSpace(
                            detail.GetString()))
                    {
                        return detail.GetString()!;
                    }

                    if (document.RootElement.TryGetProperty(
                            "title",
                            out var title)
                        && title.ValueKind
                            == JsonValueKind.String
                        && !string.IsNullOrWhiteSpace(
                            title.GetString()))
                    {
                        return title.GetString()!;
                    }
                }
            }
        }
        catch (JsonException)
        {
            return $"Server вернул HTTP {(int)response.StatusCode} ({response.ReasonPhrase}); ProblemDetails не удалось разобрать.";
        }

        return $"Server вернул HTTP {(int)response.StatusCode} ({response.ReasonPhrase}).";
    }
}

public sealed class SecurityManagementClientException : Exception
{
    public SecurityManagementClientException(
        HttpStatusCode statusCode,
        string message)
        : base(message)
    {
        StatusCode =
            statusCode;
    }

    public HttpStatusCode StatusCode { get; }
}
