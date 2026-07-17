using Dispatcher.Web.Configuration;

namespace Dispatcher.Web.Services;

public sealed class DispatcherBackendHttpClient
{
    public DispatcherBackendHttpClient(
        HttpClient httpClient,
        DispatcherApiOptions options
    )
    {
        Client = httpClient;
        Options = options;
    }

    public HttpClient Client { get; }

    public DispatcherApiOptions Options { get; }
}