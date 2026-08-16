using System.Net;
using System.Net.Http.Json;
using System.Text.Json;
using Dispatcher.Contracts.Authentication;
using Dispatcher.Contracts.Authorization;

namespace Dispatcher.Web.Services;

public sealed class AuthenticationClient
{
    private static readonly CurrentUserDto AnonymousUser =
        new(
            Authenticated: false,
            UserId: null,
            UserName: null,
            DisplayName: null,
            EffectivePermissions: Array.Empty<string>());

    private readonly HttpClient _httpClient;
    private readonly ILogger<AuthenticationClient> _logger;
    private Task? _initializationTask;

    public AuthenticationClient(
        HttpClient httpClient,
        ILogger<AuthenticationClient> logger)
    {
        _httpClient =
            httpClient;
        _logger =
            logger;
    }

    public event Action? Changed;

    public CurrentUserDto CurrentUser { get; private set; } =
        AnonymousUser;

    public bool IsInitialized { get; private set; }

    public string? LastError { get; private set; }

    public bool HasPermission(string permission)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            permission);

        if (!PermissionNames.IsKnown(
                permission))
        {
            throw new ArgumentException(
                $"Unknown permission '{permission}'.",
                nameof(permission));
        }

        return CurrentUser.Authenticated
            && CurrentUser.EffectivePermissions.Contains(
                permission,
                StringComparer.Ordinal);
    }

    public bool HasAllPermissions(
        params string[] permissions)
    {
        ArgumentNullException.ThrowIfNull(
            permissions);

        return permissions.All(
            HasPermission);
    }

    public Task InitializeAsync(
        CancellationToken cancellationToken = default)
    {
        if (IsInitialized)
        {
            return Task.CompletedTask;
        }

        _initializationTask ??=
            RefreshCoreAsync(
                initialLoad: true,
                cancellationToken);

        return _initializationTask;
    }

    public async Task<bool> LoginAsync(
        string userName,
        string password,
        CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            userName);
        ArgumentException.ThrowIfNullOrWhiteSpace(
            password);

        try
        {
            using var response =
                await _httpClient.PostAsJsonAsync(
                    "/api/auth/login",
                    new LoginRequest(
                        userName.Trim(),
                        password),
                    cancellationToken);

            if (response.StatusCode ==
                HttpStatusCode.Unauthorized)
            {
                CurrentUser =
                    AnonymousUser;
                LastError =
                    "Неверное имя пользователя или пароль.";
                IsInitialized =
                    true;
                Changed?.Invoke();

                return false;
            }

            response.EnsureSuccessStatusCode();

            var user =
                await response.Content.ReadFromJsonAsync<CurrentUserDto>(
                    cancellationToken:
                        cancellationToken);

            CurrentUser =
                ValidateAuthenticatedUser(
                    user);
            LastError =
                null;
            IsInitialized =
                true;
            Changed?.Invoke();

            return true;
        }
        catch (OperationCanceledException)
            when (cancellationToken.IsCancellationRequested)
        {
            throw;
        }
        catch (Exception exception)
            when (exception is HttpRequestException
                or JsonException
                or InvalidOperationException)
        {
            _logger.LogWarning(
                exception,
                "Failed to sign in through the local authentication API.");

            CurrentUser =
                AnonymousUser;
            LastError =
                "Не удалось выполнить вход. Проверьте доступность Server.";
            IsInitialized =
                true;
            Changed?.Invoke();

            return false;
        }
    }

    public async Task<bool> LogoutAsync(
        CancellationToken cancellationToken = default)
    {
        try
        {
            using var response =
                await _httpClient.PostAsync(
                    "/api/auth/logout",
                    content: null,
                    cancellationToken);

            response.EnsureSuccessStatusCode();

            CurrentUser =
                AnonymousUser;
            LastError =
                null;
            IsInitialized =
                true;
            Changed?.Invoke();

            return true;
        }
        catch (OperationCanceledException)
            when (cancellationToken.IsCancellationRequested)
        {
            throw;
        }
        catch (HttpRequestException exception)
        {
            _logger.LogWarning(
                exception,
                "Failed to sign out through the local authentication API.");

            LastError =
                "Не удалось завершить сессию. Проверьте доступность Server.";
            Changed?.Invoke();

            return false;
        }
    }

    public Task RefreshAsync(
        CancellationToken cancellationToken = default)
    {
        return RefreshCoreAsync(
            initialLoad: false,
            cancellationToken);
    }

    private static CurrentUserDto ValidateAuthenticatedUser(
        CurrentUserDto? user)
    {
        if (user is null
            || !user.Authenticated
            || string.IsNullOrWhiteSpace(
                user.UserId)
            || string.IsNullOrWhiteSpace(
                user.UserName)
            || string.IsNullOrWhiteSpace(
                user.DisplayName)
            || user.EffectivePermissions is null)
        {
            throw new InvalidOperationException(
                "Authentication endpoint returned an invalid authenticated user response.");
        }

        return user;
    }

    private async Task RefreshCoreAsync(
        bool initialLoad,
        CancellationToken cancellationToken)
    {
        try
        {
            var user =
                await _httpClient.GetFromJsonAsync<CurrentUserDto>(
                    "/api/auth/current",
                    cancellationToken);

            CurrentUser =
                user is { Authenticated: true }
                    ? ValidateAuthenticatedUser(
                        user)
                    : AnonymousUser;
            LastError =
                null;
        }
        catch (OperationCanceledException)
            when (cancellationToken.IsCancellationRequested)
        {
            throw;
        }
        catch (Exception exception)
            when (exception is HttpRequestException
                or JsonException
                or NotSupportedException
                or InvalidOperationException)
        {
            _logger.LogWarning(
                exception,
                "Failed to read the current local authentication session.");

            CurrentUser =
                AnonymousUser;
            LastError =
                "Не удалось проверить текущую сессию. Проверьте доступность Server.";
        }
        finally
        {
            IsInitialized =
                true;

            if (!initialLoad)
            {
                _initializationTask =
                    null;
            }

            Changed?.Invoke();
        }
    }
}
