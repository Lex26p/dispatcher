using System.Net;
using System.Net.Http.Json;
using Dispatcher.Contracts.Authentication;
using Dispatcher.Contracts.Events;
using Dispatcher.Contracts.Mimics;
using Dispatcher.Contracts.Templates;
using Dispatcher.Server.Configuration;
using Dispatcher.Server.Security;
using Microsoft.AspNetCore.Identity;
using Microsoft.AspNetCore.Mvc.Testing;
using Microsoft.Extensions.Options;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Server.Tests;

[TestClass]
public sealed class MimicTemplateApiTests
{
    private const string Password =
        "Mimic-template-test-password-42";

    [TestMethod]
    public async Task TemplateCrudAndInstantiation_CopyIndependentElementsIntoMimic()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();
        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath);
        using var client =
            factory.CreateClient();

        var target =
            CreateTargetMimic();
        var targetResponse =
            await client.PutAsJsonAsync(
                "/api/configuration/mimics/template-target",
                target);

        Assert.AreEqual(
            HttpStatusCode.OK,
            targetResponse.StatusCode);

        var template =
            CreateTemplate(
                labelText:
                    "Pump");
        var putResponse =
            await client.PutAsJsonAsync(
                "/api/configuration/mimic-templates/pump-fragment",
                template);

        Assert.AreEqual(
            HttpStatusCode.OK,
            putResponse.StatusCode);

        var listed =
            await client.GetFromJsonAsync<MimicTemplateDto[]>(
                "/api/configuration/mimic-templates");

        Assert.IsNotNull(
            listed);
        Assert.AreEqual(
            1,
            listed.Length);
        Assert.AreEqual(
            "pump-fragment",
            listed[0].TemplateId);

        var loadedTemplate =
            await client.GetFromJsonAsync<MimicTemplateDto>(
                "/api/configuration/mimic-templates/pump-fragment");

        Assert.IsNotNull(
            loadedTemplate);
        Assert.AreEqual(
            "state",
            loadedTemplate.Parameters.Single().ParameterId);
        Assert.AreEqual(
            "state",
            loadedTemplate.Elements.Single(element =>
                element.TagParameterId is not null).TagParameterId);

        var instantiateResponse =
            await client.PostAsJsonAsync(
                "/api/configuration/mimics/template-target/templates/pump-fragment/instantiate",
                new InstantiateMimicTemplateRequest(
                    X:
                        100,
                    Y:
                        50,
                    TagBindings:
                        new Dictionary<string, string>(
                            StringComparer.Ordinal)
                        {
                            ["state"] =
                                "plant.pump01.running"
                        }));

        Assert.AreEqual(
            HttpStatusCode.OK,
            instantiateResponse.StatusCode);

        var instantiated =
            await instantiateResponse.Content.ReadFromJsonAsync<MimicDefinitionDto>();

        Assert.IsNotNull(
            instantiated);
        Assert.AreEqual(
            3,
            instantiated.Elements.Count);

        var label =
            instantiated.Elements.Single(element =>
                element.Text == "Pump");
        var indicator =
            instantiated.Elements.Single(element =>
                element.TagId == "plant.pump01.running");

        Assert.AreEqual(
            110,
            label.X);
        Assert.AreEqual(
            55,
            label.Y);
        Assert.AreNotEqual(
            "label",
            label.ElementId);
        Assert.AreEqual(
            140,
            indicator.X);
        Assert.AreEqual(
            95,
            indicator.Y);
        Assert.AreNotEqual(
            "state-indicator",
            indicator.ElementId);

        var changedTemplate =
            CreateTemplate(
                labelText:
                    "Changed later");
        var updateResponse =
            await client.PutAsJsonAsync(
                "/api/configuration/mimic-templates/pump-fragment",
                changedTemplate);

        Assert.AreEqual(
            HttpStatusCode.OK,
            updateResponse.StatusCode);

        var catalog =
            await client.GetFromJsonAsync<TemplateCatalogItemDto[]>(
                "/api/configuration/templates");
        Assert.IsNotNull(
            catalog);
        var mimicCatalogItem =
            catalog.Single(item =>
                item.TemplateId == "pump-fragment");
        Assert.AreEqual(
            TemplateKindDto.Mimic,
            mimicCatalogItem.Kind);
        Assert.AreEqual(
            2,
            mimicCatalogItem.Version);

        var persistedMimic =
            await client.GetFromJsonAsync<MimicDefinitionDto>(
                "/api/mimics/template-target");

        Assert.IsNotNull(
            persistedMimic);
        Assert.IsTrue(
            persistedMimic.Elements.Any(element =>
                element.Text == "Pump"));
        Assert.IsFalse(
            persistedMimic.Elements.Any(element =>
                element.Text == "Changed later"));

        var audit =
            await WaitForTemplateAuditAsync(
                client,
                "pump-fragment");

        Assert.AreEqual(
            TestDispatcherFactory.TestAdministratorUserId,
            audit.ActorUserId);
        Assert.AreEqual(
            "dispatcher.tests.admin",
            audit.ActorUserName);

        var deleteResponse =
            await client.DeleteAsync(
                "/api/configuration/mimic-templates/pump-fragment");

        Assert.AreEqual(
            HttpStatusCode.NoContent,
            deleteResponse.StatusCode);
        Assert.AreEqual(
            HttpStatusCode.NotFound,
            (await client.GetAsync(
                "/api/configuration/mimic-templates/pump-fragment")).StatusCode);
    }

    [TestMethod]
    public async Task Instantiate_RejectsMissingOrUnknownParameterBindings()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();
        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath);
        using var client =
            factory.CreateClient();

        Assert.AreEqual(
            HttpStatusCode.OK,
            (await client.PutAsJsonAsync(
                "/api/configuration/mimics/template-target",
                CreateTargetMimic())).StatusCode);
        Assert.AreEqual(
            HttpStatusCode.OK,
            (await client.PutAsJsonAsync(
                "/api/configuration/mimic-templates/pump-fragment",
                CreateTemplate(
                    "Pump"))).StatusCode);

        var missingResponse =
            await client.PostAsJsonAsync(
                "/api/configuration/mimics/template-target/templates/pump-fragment/instantiate",
                new InstantiateMimicTemplateRequest(
                    0,
                    0,
                    new Dictionary<string, string>()));

        Assert.AreEqual(
            HttpStatusCode.BadRequest,
            missingResponse.StatusCode);

        var unknownResponse =
            await client.PostAsJsonAsync(
                "/api/configuration/mimics/template-target/templates/pump-fragment/instantiate",
                new InstantiateMimicTemplateRequest(
                    0,
                    0,
                    new Dictionary<string, string>
                    {
                        ["state"] =
                            "plant.pump01.running",
                        ["unknown"] =
                            "plant.unknown"
                    }));

        Assert.AreEqual(
            HttpStatusCode.BadRequest,
            unknownResponse.StatusCode);
    }

    [TestMethod]
    public async Task TemplateEndpoints_UseRuntimeReadForReads_AndTemplatesEditForMutations()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();

        var viewer =
            await InsertUserWithRoleAsync(
                database.DatabasePath,
                "template.viewer",
                BuiltInSecurityRoles.ViewerRoleId);
        var engineer =
            await InsertUserWithRoleAsync(
                database.DatabasePath,
                "template.engineer",
                BuiltInSecurityRoles.EngineerRoleId);

        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath,
                authenticateAsAdministrator:
                    false);
        using var viewerClient =
            CreateCookieClient(
                factory);
        using var engineerClient =
            CreateCookieClient(
                factory);

        await LoginAsync(
            viewerClient,
            viewer.UserName);
        await LoginAsync(
            engineerClient,
            engineer.UserName);

        Assert.AreEqual(
            HttpStatusCode.OK,
            (await viewerClient.GetAsync(
                "/api/configuration/mimic-templates")).StatusCode);

        Assert.AreEqual(
            HttpStatusCode.Forbidden,
            (await viewerClient.PutAsJsonAsync(
                "/api/configuration/mimic-templates/viewer-template",
                CreateTemplate(
                    "Viewer"))).StatusCode);

        Assert.AreEqual(
            HttpStatusCode.OK,
            (await engineerClient.PutAsJsonAsync(
                "/api/configuration/mimic-templates/engineer-template",
                CreateTemplate(
                    "Engineer",
                    templateId:
                        "engineer-template"))).StatusCode);
    }

    private static MimicDefinitionDto CreateTargetMimic()
    {
        return new MimicDefinitionDto(
            MimicId:
                "template-target",
            Name:
                "Template target",
            Width:
                1000,
            Height:
                700,
            Elements:
            [
                new MimicElementDto(
                    ElementId:
                        "existing",
                    Type:
                        MimicElementTypeDto.Rectangle,
                    X:
                        0,
                    Y:
                        0,
                    Width:
                        20,
                    Height:
                        20,
                    Text:
                        null,
                    TagId:
                        null,
                    CommandValue:
                        null)
            ]);
    }

    private static MimicTemplateDto CreateTemplate(
        string labelText,
        string templateId = "pump-fragment")
    {
        return new MimicTemplateDto(
            TemplateId:
                templateId,
            Name:
                "Pump fragment",
            Width:
                240,
            Height:
                120,
            Parameters:
            [
                new MimicTemplateParameterDto(
                    ParameterId:
                        "state",
                    Name:
                        "State tag")
            ],
            Elements:
            [
                new MimicTemplateElementDto(
                    ElementId:
                        "label",
                    Type:
                        MimicElementTypeDto.Text,
                    X:
                        10,
                    Y:
                        5,
                    Width:
                        100,
                    Height:
                        25,
                    Text:
                        labelText,
                    TagId:
                        null,
                    TagParameterId:
                        null,
                    CommandValue:
                        null),
                new MimicTemplateElementDto(
                    ElementId:
                        "state-indicator",
                    Type:
                        MimicElementTypeDto.Indicator,
                    X:
                        40,
                    Y:
                        45,
                    Width:
                        80,
                    Height:
                        50,
                    Text:
                        null,
                    TagId:
                        null,
                    TagParameterId:
                        "state",
                    CommandValue:
                        null)
            ]);
    }

    private static async Task<EventRecordDto> WaitForTemplateAuditAsync(
        HttpClient client,
        string templateId)
    {
        var from =
            DateTimeOffset.UtcNow.AddMinutes(
                -2);

        for (var attempt = 0; attempt < 100; attempt++)
        {
            var to =
                DateTimeOffset.UtcNow.AddMinutes(
                    1);
            var url =
                $"/api/events?from={Uri.EscapeDataString(from.ToString("O"))}&to={Uri.EscapeDataString(to.ToString("O"))}&page=1&limit=200";
            var response =
                await client.GetFromJsonAsync<EventQueryResponseDto>(
                    url);
            var match =
                response?.Items.FirstOrDefault(record =>
                    record.Type == "ConfigurationChanged"
                    && record.DataJson?.Contains(
                        templateId,
                        StringComparison.Ordinal) == true
                    && record.ActorUserId
                        == TestDispatcherFactory.TestAdministratorUserId);

            if (match is not null)
            {
                return match;
            }

            await Task.Delay(
                20);
        }

        Assert.Fail(
            $"Actor-aware template audit for '{templateId}' was not persisted in time.");
        throw new InvalidOperationException();
    }

    private static HttpClient CreateCookieClient(
        WebApplicationFactory<Program> factory)
    {
        return factory.CreateClient(
            new WebApplicationFactoryClientOptions
            {
                AllowAutoRedirect =
                    false,
                HandleCookies =
                    true
            });
    }

    private static async Task LoginAsync(
        HttpClient client,
        string userName)
    {
        var response =
            await client.PostAsJsonAsync(
                "/api/auth/login",
                new LoginRequest(
                    UserName:
                        userName,
                    Password:
                        Password));

        Assert.AreEqual(
            HttpStatusCode.OK,
            response.StatusCode);
    }

    private static async Task<LocalUserConfiguration> InsertUserWithRoleAsync(
        string databasePath,
        string userName,
        string roleId)
    {
        var store =
            new SqliteConfigurationStore(
                databasePath);
        var role =
            BuiltInSecurityRoles.All.Single(candidate =>
                candidate.RoleId == roleId);
        var user =
            new LocalUserConfiguration(
                UserId:
                    Guid.NewGuid().ToString(
                        "N"),
                UserName:
                    userName,
                NormalizedUserName:
                    LocalUserConfiguration.NormalizeUserName(
                        userName),
                DisplayName:
                    userName,
                Enabled:
                    true,
                PasswordHash:
                    "pending");
        var hasher =
            new PasswordHasher<LocalUserConfiguration>(
                Options.Create(
                    new PasswordHasherOptions()));

        user =
            user with
            {
                PasswordHash =
                    hasher.HashPassword(
                        user,
                        Password)
            };

        await store.InsertLocalUserWithRoleAsync(
            user,
            role);

        return user;
    }
}
