namespace Dispatcher.Server.Security;

public sealed class SecurityManagementNotFoundException : Exception
{
    public SecurityManagementNotFoundException(string message)
        : base(message)
    {
    }
}

public sealed class SecurityManagementConflictException : Exception
{
    public SecurityManagementConflictException(string message)
        : base(message)
    {
    }
}
