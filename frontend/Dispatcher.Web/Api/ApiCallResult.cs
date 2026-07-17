namespace Dispatcher.Web.Api;

public sealed class ApiCallResult<T>
{
    private ApiCallResult(
        bool success,
        T? value,
        string? errorMessage
    )
    {
        Success = success;
        Value = value;
        ErrorMessage = errorMessage;
    }

    public bool Success { get; }

    public bool Failed => !Success;

    public T? Value { get; }

    public string? ErrorMessage { get; }

    public bool HasValue => Value is not null;

    public static ApiCallResult<T> Ok(
        T value
    )
    {
        return new ApiCallResult<T>(
            true,
            value,
            null
        );
    }

    public static ApiCallResult<T> Fail(
        string errorMessage
    )
    {
        return new ApiCallResult<T>(
            false,
            default,
            string.IsNullOrWhiteSpace(errorMessage)
                ? "API call failed."
                : errorMessage
        );
    }
}