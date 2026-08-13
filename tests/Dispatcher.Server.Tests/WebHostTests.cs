using System.Net;
using System.Text.RegularExpressions;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Server.Tests;

[TestClass]
public sealed partial class WebHostTests
{
    [TestMethod]
    public async Task Root_ReturnsBlazorWebAssemblyShell()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();
        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath);
        using var client =
            factory.CreateClient();

        var response =
            await client.GetAsync("/");

        Assert.AreEqual(
            HttpStatusCode.OK,
            response.StatusCode);

        var html =
            await response.Content.ReadAsStringAsync();

        StringAssert.Contains(
            html,
            "<div id=\"app\">");
        StringAssert.Contains(
            html,
            "<script type=\"importmap\">");

        var scriptMatch =
            BlazorBootstrapScriptRegex().Match(html);

        Assert.IsTrue(
            scriptMatch.Success,
            "The generated page must reference the fingerprinted Blazor WebAssembly bootstrap script.");

        var scriptPath =
            scriptMatch.Groups["path"].Value;

        Assert.IsFalse(
            scriptPath.Contains(
                "#[.{fingerprint}]",
                StringComparison.Ordinal),
            "The fingerprint placeholder must be replaced in the generated HTML.");

        var requestPath =
            scriptPath.StartsWith(
                "/",
                StringComparison.Ordinal)
                ? scriptPath
                : "/" + scriptPath;

        var scriptResponse =
            await client.GetAsync(
                requestPath);

        Assert.AreEqual(
            HttpStatusCode.OK,
            scriptResponse.StatusCode,
            $"Blazor bootstrap script '{requestPath}' must be served by ASP.NET Core.");
    }

    [GeneratedRegex(
        "src=[\"'](?<path>/?_framework/blazor\\.webassembly(?:\\.[^\"']+)?\\.js)[\"']",
        RegexOptions.IgnoreCase)]
    private static partial Regex BlazorBootstrapScriptRegex();
}
