using Dispatcher.Web;
using Dispatcher.Web.Api;
using Dispatcher.Web.Configuration;
using Dispatcher.Web.Services;
using Microsoft.AspNetCore.Components.Web;
using Microsoft.AspNetCore.Components.WebAssembly.Hosting;
using MudBlazor.Services;
using Dispatcher.Web.Components.Shell;
using Dispatcher.Web.MockData;

var builder = WebAssemblyHostBuilder.CreateDefault(args);

builder.RootComponents.Add<App>("#app");
builder.RootComponents.Add<HeadOutlet>("head::after");

builder.Services.AddMudServices();

var dispatcherApiOptions =
    DispatcherApiOptions.FromConfiguration(
        builder.Configuration
    );

builder.Services.AddSingleton(
    dispatcherApiOptions
);

builder.Services.AddScoped(
    _ => new HttpClient
    {
        BaseAddress = new Uri(builder.HostEnvironment.BaseAddress)
    }
);

builder.Services.AddScoped(
    serviceProvider =>
    {
        var options =
            serviceProvider.GetRequiredService<DispatcherApiOptions>();

        var httpClient =
            new HttpClient
            {
                BaseAddress = options.BaseUri,
                Timeout = options.Timeout
            };

        return new DispatcherBackendHttpClient(
            httpClient,
            options
        );
    }
);

builder.Services.AddScoped<DispatcherBackendApiClient>();
builder.Services.AddScoped<DispatcherShellState>();
builder.Services.AddSingleton<DispatcherMockDataService>();
builder.Services.AddSingleton<MockDeviceDiagnosticsService>();

await builder.Build().RunAsync();