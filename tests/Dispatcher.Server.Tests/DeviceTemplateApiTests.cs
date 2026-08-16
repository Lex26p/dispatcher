using System.Net;
using System.Net.Http.Json;
using Dispatcher.Contracts.Authentication;
using Dispatcher.Contracts.Authorization;
using Dispatcher.Contracts.Configuration;
using Dispatcher.Contracts.Events;
using Dispatcher.Contracts.Templates;
using Dispatcher.Server.Configuration;
using Dispatcher.Server.Security;
using Microsoft.AspNetCore.Identity;
using Microsoft.AspNetCore.Mvc.Testing;
using Microsoft.Extensions.Options;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Server.Tests;

[TestClass]
public sealed class DeviceTemplateApiTests
{
    private const string Password =
        "Device-template-test-password-42";

    [TestMethod]
    public async Task DeviceTemplates_CreateCatalogVersions_AndInstantiateProtocolConfiguration()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();
        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath);
        using var client =
            factory.CreateClient();

        var modbus =
            CreateModbusTemplate(
                name: "Pump Modbus");
        var firstPut =
            await client.PutAsJsonAsync(
                "/api/configuration/templates/modbus-devices/pump-modbus",
                modbus);

        Assert.AreEqual(
            HttpStatusCode.OK,
            firstPut.StatusCode);
        var firstSaved =
            await firstPut.Content.ReadFromJsonAsync<ModbusDeviceTemplateDto>();
        Assert.IsNotNull(
            firstSaved);
        Assert.AreEqual(
            1,
            firstSaved.Version);

        var secondPut =
            await client.PutAsJsonAsync(
                "/api/configuration/templates/modbus-devices/pump-modbus",
                CreateModbusTemplate(
                    name: "Pump Modbus v2"));
        Assert.AreEqual(
            HttpStatusCode.OK,
            secondPut.StatusCode);
        var secondSaved =
            await secondPut.Content.ReadFromJsonAsync<ModbusDeviceTemplateDto>();
        Assert.IsNotNull(
            secondSaved);
        Assert.AreEqual(
            2,
            secondSaved.Version);

        var modbusInstance =
            await client.PostAsJsonAsync(
                "/api/configuration/templates/modbus-devices/pump-modbus/instantiate",
                new InstantiateDeviceTemplateRequest(
                    "pump01",
                    new Dictionary<string, string>(StringComparer.Ordinal)
                    {
                        ["deviceName"] = "Pump 01",
                        ["host"] = "127.0.0.1",
                        ["tagPrefix"] = "plant.pump01."
                    }));
        Assert.AreEqual(
            HttpStatusCode.Created,
            modbusInstance.StatusCode);
        var modbusDevice =
            await modbusInstance.Content.ReadFromJsonAsync<ModbusDeviceConfigurationDto>();
        Assert.IsNotNull(
            modbusDevice);
        Assert.AreEqual(
            "Pump 01",
            modbusDevice.Name);
        Assert.AreEqual(
            "127.0.0.1",
            modbusDevice.Host);
        Assert.AreEqual(
            17,
            modbusDevice.UnitId);
        Assert.AreEqual(
            "plant.pump01.running",
            modbusDevice.Tags[0].TagId);
        Assert.AreEqual(
            100,
            modbusDevice.Tags[0].Address);
        Assert.IsTrue(
            modbusDevice.Tags[0].Writable);

        var snmpPut =
            await client.PutAsJsonAsync(
                "/api/configuration/templates/snmp-devices/switch-snmp",
                CreateSnmpTemplate());
        Assert.AreEqual(
            HttpStatusCode.OK,
            snmpPut.StatusCode);

        var crossProtocolConflict =
            await client.PostAsJsonAsync(
                "/api/configuration/templates/snmp-devices/switch-snmp/instantiate",
                new InstantiateDeviceTemplateRequest(
                    "pump01",
                    new Dictionary<string, string>(StringComparer.Ordinal)
                    {
                        ["deviceName"] = "Duplicate device",
                        ["host"] = "127.0.0.2",
                        ["community"] = "public",
                        ["tagPrefix"] = "duplicate."
                    }));
        Assert.AreEqual(
            HttpStatusCode.Conflict,
            crossProtocolConflict.StatusCode);

        var snmpInstance =
            await client.PostAsJsonAsync(
                "/api/configuration/templates/snmp-devices/switch-snmp/instantiate",
                new InstantiateDeviceTemplateRequest(
                    "switch01",
                    new Dictionary<string, string>(StringComparer.Ordinal)
                    {
                        ["deviceName"] = "Switch 01",
                        ["host"] = "127.0.0.2",
                        ["community"] = "private-community",
                        ["tagPrefix"] = "plant.switch01."
                    }));
        Assert.AreEqual(
            HttpStatusCode.Created,
            snmpInstance.StatusCode);
        var snmpDevice =
            await snmpInstance.Content.ReadFromJsonAsync<SnmpDeviceConfigurationDto>();
        Assert.IsNotNull(
            snmpDevice);
        Assert.AreEqual(
            "Switch 01",
            snmpDevice.Name);
        Assert.AreEqual(
            "private-community",
            snmpDevice.Community);
        Assert.AreEqual(
            "plant.switch01.uptime",
            snmpDevice.Tags[0].TagId);
        Assert.AreEqual(
            "1.3.6.1.2.1.1.3.0",
            snmpDevice.Tags[0].Oid);

        var catalog =
            await client.GetFromJsonAsync<TemplateCatalogItemDto[]>(
                "/api/configuration/templates");
        Assert.IsNotNull(
            catalog);
        Assert.AreEqual(
            2,
            catalog.Length);
        Assert.AreEqual(
            TemplateKindDto.ModbusDevice,
            catalog.Single(item => item.TemplateId == "pump-modbus").Kind);
        Assert.AreEqual(
            2,
            catalog.Single(item => item.TemplateId == "pump-modbus").Version);
        CollectionAssert.AreEquivalent(
            new[] { "deviceName", "host", "tagPrefix" },
            catalog.Single(item => item.TemplateId == "pump-modbus")
                .Parameters
                .Select(parameter => parameter.ParameterId)
                .ToArray());
        Assert.AreEqual(
            TemplateKindDto.SnmpDevice,
            catalog.Single(item => item.TemplateId == "switch-snmp").Kind);

        var modbusList =
            await client.GetFromJsonAsync<ModbusDeviceConfigurationDto[]>(
                "/api/configuration/modbus/devices");
        var snmpList =
            await client.GetFromJsonAsync<SnmpDeviceConfigurationDto[]>(
                "/api/configuration/snmp/devices");
        Assert.IsNotNull(modbusList);
        Assert.IsNotNull(snmpList);
        Assert.IsTrue(
            modbusList.Any(device => device.DeviceId == "pump01"));
        Assert.IsTrue(
            snmpList.Any(device => device.DeviceId == "switch01"));

        var changedTemplateResponse =
            await client.PutAsJsonAsync(
                "/api/configuration/templates/modbus-devices/pump-modbus",
                CreateModbusTemplate(
                    name: "Changed after instantiate",
                    firstAddress: 900));
        Assert.AreEqual(
            HttpStatusCode.OK,
            changedTemplateResponse.StatusCode);
        var changedTemplate =
            await changedTemplateResponse.Content.ReadFromJsonAsync<ModbusDeviceTemplateDto>();
        Assert.IsNotNull(
            changedTemplate);
        Assert.AreEqual(
            3,
            changedTemplate.Version);

        var persistedDevice =
            (await client.GetFromJsonAsync<ModbusDeviceConfigurationDto[]>(
                "/api/configuration/modbus/devices"))
            ?.Single(device => device.DeviceId == "pump01");
        Assert.IsNotNull(
            persistedDevice);
        Assert.AreEqual(
            100,
            persistedDevice.Tags[0].Address);

        Assert.AreEqual(
            HttpStatusCode.NoContent,
            (await client.DeleteAsync(
                "/api/configuration/templates/modbus-devices/pump-modbus")).StatusCode);
        Assert.AreEqual(
            HttpStatusCode.NotFound,
            (await client.GetAsync(
                "/api/configuration/templates/modbus-devices/pump-modbus")).StatusCode);
        Assert.IsTrue(
            (await client.GetFromJsonAsync<ModbusDeviceConfigurationDto[]>(
                "/api/configuration/modbus/devices"))
            ?.Any(device => device.DeviceId == "pump01") == true);

        var audit =
            await WaitForAuditAsync(
                client,
                "pump01");
        Assert.AreEqual(
            TestDispatcherFactory.TestAdministratorUserId,
            audit.ActorUserId);
        Assert.AreEqual(
            "dispatcher.tests.admin",
            audit.ActorUserName);
    }

    [TestMethod]
    public async Task TemplateId_IsUniqueAcrossKinds_AndInstanceParametersAreExact()
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
                "/api/configuration/templates/modbus-devices/shared-template",
                CreateModbusTemplate(
                    templateId: "shared-template"))).StatusCode);

        Assert.AreEqual(
            HttpStatusCode.Conflict,
            (await client.PutAsJsonAsync(
                "/api/configuration/templates/snmp-devices/shared-template",
                CreateSnmpTemplate(
                    templateId: "shared-template"))).StatusCode);

        var missing =
            await client.PostAsJsonAsync(
                "/api/configuration/templates/modbus-devices/shared-template/instantiate",
                new InstantiateDeviceTemplateRequest(
                    "missing-values",
                    new Dictionary<string, string>
                    {
                        ["host"] = "127.0.0.1",
                        ["tagPrefix"] = "missing."
                    }));
        Assert.AreEqual(
            HttpStatusCode.BadRequest,
            missing.StatusCode);

        var unknown =
            await client.PostAsJsonAsync(
                "/api/configuration/templates/modbus-devices/shared-template/instantiate",
                new InstantiateDeviceTemplateRequest(
                    "unknown-values",
                    new Dictionary<string, string>
                    {
                        ["deviceName"] = "Unknown",
                        ["host"] = "127.0.0.1",
                        ["tagPrefix"] = "unknown.",
                        ["extra"] = "not-declared"
                    }));
        Assert.AreEqual(
            HttpStatusCode.BadRequest,
            unknown.StatusCode);
    }

    [TestMethod]
    public async Task TemplateMutationAndInstantiation_UseIndependentPermissions()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();

        var deviceInstantiator =
            await InsertUserWithPermissionsAsync(
                database.DatabasePath,
                "device.template.user",
                [
                    PermissionNames.RuntimeRead,
                    PermissionNames.DevicesEdit
                ]);
        var templateEditor =
            await InsertUserWithPermissionsAsync(
                database.DatabasePath,
                "template.editor.user",
                [
                    PermissionNames.RuntimeRead,
                    PermissionNames.TemplatesEdit
                ]);

        using (var adminFactory =
            TestDispatcherFactory.Create(
                database.DatabasePath))
        using (var adminClient =
            adminFactory.CreateClient())
        {
            Assert.AreEqual(
                HttpStatusCode.OK,
                (await adminClient.PutAsJsonAsync(
                    "/api/configuration/templates/modbus-devices/approved-template",
                    CreateModbusTemplate(
                        templateId: "approved-template"))).StatusCode);
        }

        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath,
                authenticateAsAdministrator: false);
        using var instantiatorClient =
            CreateCookieClient(factory);
        using var editorClient =
            CreateCookieClient(factory);

        await LoginAsync(
            instantiatorClient,
            deviceInstantiator.UserName);
        await LoginAsync(
            editorClient,
            templateEditor.UserName);

        Assert.AreEqual(
            HttpStatusCode.OK,
            (await instantiatorClient.GetAsync(
                "/api/configuration/templates")).StatusCode);
        Assert.AreEqual(
            HttpStatusCode.OK,
            (await instantiatorClient.GetAsync(
                "/api/configuration/templates/modbus-devices/approved-template")).StatusCode);
        Assert.AreEqual(
            HttpStatusCode.Forbidden,
            (await instantiatorClient.PutAsJsonAsync(
                "/api/configuration/templates/modbus-devices/approved-template",
                CreateModbusTemplate(
                    templateId: "approved-template"))).StatusCode);
        Assert.AreEqual(
            HttpStatusCode.Created,
            (await instantiatorClient.PostAsJsonAsync(
                "/api/configuration/templates/modbus-devices/approved-template/instantiate",
                CreateInstanceRequest(
                    "device-from-approved",
                    "approved."))).StatusCode);

        Assert.AreEqual(
            HttpStatusCode.OK,
            (await editorClient.PutAsJsonAsync(
                "/api/configuration/templates/modbus-devices/editor-template",
                CreateModbusTemplate(
                    templateId: "editor-template"))).StatusCode);
        Assert.AreEqual(
            HttpStatusCode.Forbidden,
            (await editorClient.PostAsJsonAsync(
                "/api/configuration/templates/modbus-devices/editor-template/instantiate",
                CreateInstanceRequest(
                    "forbidden-device",
                    "forbidden."))).StatusCode);
    }

    private static ModbusDeviceTemplateUpsertRequest CreateModbusTemplate(
        string templateId = "pump-modbus",
        string name = "Pump Modbus",
        int firstAddress = 100)
    {
        return new ModbusDeviceTemplateUpsertRequest(
            templateId,
            name,
            CreateCommonParameters(),
            DeviceName: "Pump",
            DeviceNameParameterId: "deviceName",
            HostParameterId: "host",
            TagIdPrefixParameterId: "tagPrefix",
            Enabled: false,
            Port: 502,
            UnitId: 17,
            PollIntervalMilliseconds: 1000,
            RequestTimeoutMilliseconds: 1000,
            Tags:
            [
                new ModbusTagTemplateDto(
                    "running",
                    "Running",
                    firstAddress,
                    true),
                new ModbusTagTemplateDto(
                    "speed",
                    "Speed",
                    101,
                    false)
            ]);
    }

    private static SnmpDeviceTemplateUpsertRequest CreateSnmpTemplate(
        string templateId = "switch-snmp")
    {
        return new SnmpDeviceTemplateUpsertRequest(
            templateId,
            "Switch SNMP",
            CreateCommonParameters()
                .Append(
                    new TemplateParameterDto(
                        "community",
                        "Community"))
                .ToArray(),
            DeviceName: "Switch",
            DeviceNameParameterId: "deviceName",
            HostParameterId: "host",
            CommunityParameterId: "community",
            TagIdPrefixParameterId: "tagPrefix",
            Enabled: false,
            Port: 161,
            PollIntervalMilliseconds: 1000,
            RequestTimeoutMilliseconds: 1000,
            Tags:
            [
                new SnmpTagTemplateDto(
                    "uptime",
                    "Uptime",
                    "1.3.6.1.2.1.1.3.0")
            ]);
    }

    private static TemplateParameterDto[] CreateCommonParameters()
    {
        return
        [
            new TemplateParameterDto(
                "deviceName",
                "Device name"),
            new TemplateParameterDto(
                "host",
                "Host"),
            new TemplateParameterDto(
                "tagPrefix",
                "Tag ID prefix")
        ];
    }

    private static InstantiateDeviceTemplateRequest CreateInstanceRequest(
        string deviceId,
        string prefix)
    {
        return new InstantiateDeviceTemplateRequest(
            deviceId,
            new Dictionary<string, string>(StringComparer.Ordinal)
            {
                ["deviceName"] = deviceId,
                ["host"] = "127.0.0.1",
                ["tagPrefix"] = prefix
            });
    }

    private static async Task<EventRecordDto> WaitForAuditAsync(
        HttpClient client,
        string deviceId)
    {
        var from =
            DateTimeOffset.UtcNow.AddMinutes(-2);

        for (var attempt = 0; attempt < 100; attempt++)
        {
            var to =
                DateTimeOffset.UtcNow.AddMinutes(1);
            var url =
                $"/api/events?from={Uri.EscapeDataString(from.ToString("O"))}&to={Uri.EscapeDataString(to.ToString("O"))}&page=1&limit=200";
            var response =
                await client.GetFromJsonAsync<EventQueryResponseDto>(
                    url);
            var match =
                response?.Items.FirstOrDefault(record =>
                    record.Type == "ConfigurationChanged"
                    && record.DataJson?.Contains(
                        deviceId,
                        StringComparison.Ordinal) == true
                    && record.DataJson?.Contains(
                        "InstantiateTemplate",
                        StringComparison.Ordinal) == true);

            if (match is not null)
            {
                return match;
            }

            await Task.Delay(20);
        }

        Assert.Fail(
            $"Actor-aware device template audit for '{deviceId}' was not persisted in time.");
        throw new InvalidOperationException();
    }

    private static HttpClient CreateCookieClient(
        WebApplicationFactory<Program> factory)
    {
        return factory.CreateClient(
            new WebApplicationFactoryClientOptions
            {
                AllowAutoRedirect = false,
                HandleCookies = true
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
                    userName,
                    Password));
        Assert.AreEqual(
            HttpStatusCode.OK,
            response.StatusCode);
    }

    private static async Task<LocalUserConfiguration> InsertUserWithPermissionsAsync(
        string databasePath,
        string userName,
        IReadOnlyList<string> permissions)
    {
        var store =
            new SqliteConfigurationStore(
                databasePath);
        var role =
            new SecurityRoleConfiguration(
                RoleId: Guid.NewGuid().ToString("N"),
                Name: $"Role {userName}",
                NormalizedName: SecurityRoleConfiguration.NormalizeName(
                    $"Role {userName}"),
                BuiltIn: false,
                Permissions: permissions);
        var user =
            new LocalUserConfiguration(
                UserId: Guid.NewGuid().ToString("N"),
                UserName: userName,
                NormalizedUserName: LocalUserConfiguration.NormalizeUserName(userName),
                DisplayName: userName,
                Enabled: true,
                PasswordHash: "pending");
        var hasher =
            new PasswordHasher<LocalUserConfiguration>(
                Options.Create(
                    new PasswordHasherOptions()));
        user =
            user with
            {
                PasswordHash = hasher.HashPassword(
                    user,
                    Password)
            };

        await store.InsertLocalUserWithRoleAsync(
            user,
            role);

        return user;
    }
}
