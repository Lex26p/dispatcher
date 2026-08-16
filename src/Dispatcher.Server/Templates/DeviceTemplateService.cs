using Dispatcher.Contracts.Templates;
using Dispatcher.Server.Configuration;

namespace Dispatcher.Server.Templates;

public sealed class DeviceTemplateService
{
    private readonly SqliteConfigurationStore _store;
    private readonly ConfigurationEditorService _configurationEditor;
    private readonly TemplateMutationGate _mutationGate;

    public DeviceTemplateService(
        SqliteConfigurationStore store,
        ConfigurationEditorService configurationEditor,
        TemplateMutationGate mutationGate)
    {
        _store = store;
        _configurationEditor = configurationEditor;
        _mutationGate = mutationGate;
    }

    public Task<IReadOnlyList<ModbusDeviceTemplateConfiguration>> GetModbusTemplatesAsync(
        CancellationToken cancellationToken)
    {
        return _store.LoadModbusDeviceTemplatesAsync(
            cancellationToken);
    }

    public async Task<ModbusDeviceTemplateConfiguration?> GetModbusTemplateAsync(
        string templateId,
        CancellationToken cancellationToken)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            templateId);

        return (await GetModbusTemplatesAsync(
                cancellationToken))
            .FirstOrDefault(template =>
                string.Equals(
                    template.TemplateId,
                    templateId,
                    StringComparison.Ordinal));
    }

    public Task<IReadOnlyList<SnmpDeviceTemplateConfiguration>> GetSnmpTemplatesAsync(
        CancellationToken cancellationToken)
    {
        return _store.LoadSnmpDeviceTemplatesAsync(
            cancellationToken);
    }

    public async Task<SnmpDeviceTemplateConfiguration?> GetSnmpTemplateAsync(
        string templateId,
        CancellationToken cancellationToken)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            templateId);

        return (await GetSnmpTemplatesAsync(
                cancellationToken))
            .FirstOrDefault(template =>
                string.Equals(
                    template.TemplateId,
                    templateId,
                    StringComparison.Ordinal));
    }

    public async Task<ModbusDeviceTemplateConfiguration> UpsertModbusAsync(
        string templateId,
        ModbusDeviceTemplateConfiguration template,
        CancellationToken cancellationToken)
    {
        EnsureMatchingId(
            templateId,
            template.TemplateId);
        DeviceTemplateConfigurationValidator.Validate(
            template);

        await _mutationGate.Semaphore.WaitAsync(
            cancellationToken);
        try
        {
            return await _store.UpsertModbusDeviceTemplateAsync(
                template,
                cancellationToken);
        }
        finally
        {
            _mutationGate.Semaphore.Release();
        }
    }

    public async Task<SnmpDeviceTemplateConfiguration> UpsertSnmpAsync(
        string templateId,
        SnmpDeviceTemplateConfiguration template,
        CancellationToken cancellationToken)
    {
        EnsureMatchingId(
            templateId,
            template.TemplateId);
        DeviceTemplateConfigurationValidator.Validate(
            template);

        await _mutationGate.Semaphore.WaitAsync(
            cancellationToken);
        try
        {
            return await _store.UpsertSnmpDeviceTemplateAsync(
                template,
                cancellationToken);
        }
        finally
        {
            _mutationGate.Semaphore.Release();
        }
    }

    public async Task<bool> DeleteModbusAsync(
        string templateId,
        CancellationToken cancellationToken)
    {
        return await DeleteAsync(
            templateId,
            TemplateKind.ModbusDevice,
            cancellationToken);
    }

    public async Task<bool> DeleteSnmpAsync(
        string templateId,
        CancellationToken cancellationToken)
    {
        return await DeleteAsync(
            templateId,
            TemplateKind.SnmpDevice,
            cancellationToken);
    }

    public async Task<(ModbusDeviceConfiguration Device, int TemplateVersion)> InstantiateModbusAsync(
        string templateId,
        InstantiateDeviceTemplateRequest request,
        CancellationToken cancellationToken)
    {
        ArgumentNullException.ThrowIfNull(
            request);
        ArgumentException.ThrowIfNullOrWhiteSpace(
            request.DeviceId);
        ArgumentNullException.ThrowIfNull(
            request.ParameterValues);

        var template =
            await GetModbusTemplateAsync(
                templateId,
                cancellationToken)
            ?? throw new KeyNotFoundException(
                $"Modbus device template '{templateId}' was not found.");
        var entry = ToCatalogEntry(
            template);
        var values =
            TemplateConfigurationValidator.ValidateInstanceParameters(
                entry,
                request.ParameterValues);
        var prefix =
            values[template.TagIdPrefixParameterId];

        var device =
            new ModbusDeviceConfiguration(
                request.DeviceId.Trim(),
                ResolveDeviceName(
                    template.DeviceName,
                    template.DeviceNameParameterId,
                    values),
                template.Enabled,
                values[template.HostParameterId],
                template.Port,
                template.UnitId,
                template.PollIntervalMilliseconds,
                template.RequestTimeoutMilliseconds,
                template.Tags
                    .Select(tag =>
                        new ModbusTagConfiguration(
                            prefix + tag.TagIdSuffix,
                            tag.Name,
                            tag.Address,
                            tag.Writable))
                    .ToArray());

        var created =
            await _configurationEditor.CreateDeviceConfigurationAsync(
                device,
                cancellationToken);

        return (created, template.Version);
    }

    public async Task<(SnmpDeviceConfiguration Device, int TemplateVersion)> InstantiateSnmpAsync(
        string templateId,
        InstantiateDeviceTemplateRequest request,
        CancellationToken cancellationToken)
    {
        ArgumentNullException.ThrowIfNull(
            request);
        ArgumentException.ThrowIfNullOrWhiteSpace(
            request.DeviceId);
        ArgumentNullException.ThrowIfNull(
            request.ParameterValues);

        var template =
            await GetSnmpTemplateAsync(
                templateId,
                cancellationToken)
            ?? throw new KeyNotFoundException(
                $"SNMP device template '{templateId}' was not found.");
        var entry = ToCatalogEntry(
            template);
        var values =
            TemplateConfigurationValidator.ValidateInstanceParameters(
                entry,
                request.ParameterValues);
        var prefix =
            values[template.TagIdPrefixParameterId];

        var device =
            new SnmpDeviceConfiguration(
                request.DeviceId.Trim(),
                ResolveDeviceName(
                    template.DeviceName,
                    template.DeviceNameParameterId,
                    values),
                template.Enabled,
                values[template.HostParameterId],
                template.Port,
                values[template.CommunityParameterId],
                template.PollIntervalMilliseconds,
                template.RequestTimeoutMilliseconds,
                template.Tags
                    .Select(tag =>
                        new SnmpTagConfiguration(
                            prefix + tag.TagIdSuffix,
                            tag.Name,
                            tag.Oid))
                    .ToArray());

        var created =
            await _configurationEditor.CreateSnmpDeviceConfigurationAsync(
                device,
                cancellationToken);

        return (created, template.Version);
    }

    private async Task<bool> DeleteAsync(
        string templateId,
        TemplateKind kind,
        CancellationToken cancellationToken)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            templateId);

        await _mutationGate.Semaphore.WaitAsync(
            cancellationToken);
        try
        {
            return await _store.DeleteTemplateAsync(
                templateId,
                kind,
                cancellationToken);
        }
        finally
        {
            _mutationGate.Semaphore.Release();
        }
    }

    private static void EnsureMatchingId(
        string pathTemplateId,
        string bodyTemplateId)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            pathTemplateId);

        if (!string.Equals(
                pathTemplateId,
                bodyTemplateId,
                StringComparison.Ordinal))
        {
            throw new InvalidOperationException(
                "TemplateId in URL must match TemplateId in request body.");
        }
    }

    private static string ResolveDeviceName(
        string defaultName,
        string? parameterId,
        IReadOnlyDictionary<string, string> values)
    {
        return string.IsNullOrWhiteSpace(
                parameterId)
            ? defaultName
            : values[parameterId];
    }

    private static TemplateCatalogEntryConfiguration ToCatalogEntry(
        ModbusDeviceTemplateConfiguration template)
    {
        return new TemplateCatalogEntryConfiguration(
            template.TemplateId,
            template.Name,
            TemplateKind.ModbusDevice,
            template.Version,
            template.Parameters);
    }

    private static TemplateCatalogEntryConfiguration ToCatalogEntry(
        SnmpDeviceTemplateConfiguration template)
    {
        return new TemplateCatalogEntryConfiguration(
            template.TemplateId,
            template.Name,
            TemplateKind.SnmpDevice,
            template.Version,
            template.Parameters);
    }
}
