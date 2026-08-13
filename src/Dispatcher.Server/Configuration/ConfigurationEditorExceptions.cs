namespace Dispatcher.Server.Configuration;

public sealed class ConfigurationNotFoundException : Exception
{
    public ConfigurationNotFoundException(string message)
        : base(message)
    {
    }
}

public sealed class ConfigurationConflictException : Exception
{
    public ConfigurationConflictException(string message)
        : base(message)
    {
    }
}
