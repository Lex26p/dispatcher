using Dispatcher.Web;
using Dispatcher.Web.Services;
using Microsoft.AspNetCore.Components.Web;
using Microsoft.AspNetCore.Components.WebAssembly.Hosting;

var builder = WebAssemblyHostBuilder.CreateDefault(args);

builder.RootComponents.Add<App>("#app");
builder.RootComponents.Add<HeadOutlet>("head::after");

builder.Services.AddScoped(_ => new HttpClient
{
    BaseAddress = new Uri(builder.HostEnvironment.BaseAddress)
});

builder.Services.AddScoped<AuthenticationClient>();
builder.Services.AddScoped<SecurityManagementClient>();
builder.Services.AddScoped<RuntimeStateClient>();
builder.Services.AddScoped<ConfigurationClient>();
builder.Services.AddScoped<MimicClient>();
builder.Services.AddScoped<HistoryClient>();
builder.Services.AddScoped<EventClient>();

await builder.Build().RunAsync();
