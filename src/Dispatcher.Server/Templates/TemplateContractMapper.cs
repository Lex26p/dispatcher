using Dispatcher.Contracts.Configuration;
using Dispatcher.Contracts.Templates;
using Dispatcher.Server.Configuration;

namespace Dispatcher.Server.Templates;

internal static class TemplateContractMapper
{
    public static TemplateCatalogItemDto ToDto(
        TemplateCatalogEntryConfiguration entry)
    {
        return new TemplateCatalogItemDto(
            entry.TemplateId,
            entry.Name,
            MapKind(
                entry.Kind),
            entry.Version,
            entry.Parameters
                .Select(ToDto)
                .ToArray());
    }

    public static ModbusDeviceTemplateDto ToDto(
        ModbusDeviceTemplateConfiguration template)
    {
        return new ModbusDeviceTemplateDto(
            template.TemplateId,
            template.Name,
            template.Version,
            template.Parameters.Select(ToDto).ToArray(),
            template.DeviceName,
            template.DeviceNameParameterId,
            template.HostParameterId,
            template.TagIdPrefixParameterId,
            template.Enabled,
            template.Port,
            template.UnitId,
            template.PollIntervalMilliseconds,
            template.RequestTimeoutMilliseconds,
            template.Tags
                .Select(tag =>
                    new ModbusTagTemplateDto(
                        tag.TagIdSuffix,
                        tag.Name,
                        tag.Address,
                        tag.Writable))
                .ToArray());
    }

    public static SnmpDeviceTemplateDto ToDto(
        SnmpDeviceTemplateConfiguration template)
    {
        return new SnmpDeviceTemplateDto(
            template.TemplateId,
            template.Name,
            template.Version,
            template.Parameters.Select(ToDto).ToArray(),
            template.DeviceName,
            template.DeviceNameParameterId,
            template.HostParameterId,
            template.CommunityParameterId,
            template.TagIdPrefixParameterId,
            template.Enabled,
            template.Port,
            template.PollIntervalMilliseconds,
            template.RequestTimeoutMilliseconds,
            template.Tags
                .Select(tag =>
                    new SnmpTagTemplateDto(
                        tag.TagIdSuffix,
                        tag.Name,
                        tag.Oid))
                .ToArray());
    }

    public static ModbusDeviceTemplateConfiguration ToConfiguration(
        ModbusDeviceTemplateUpsertRequest request)
    {
        ArgumentNullException.ThrowIfNull(
            request);
        ArgumentNullException.ThrowIfNull(
            request.Parameters);
        ArgumentNullException.ThrowIfNull(
            request.Tags);

        return new ModbusDeviceTemplateConfiguration(
            request.TemplateId,
            request.Name,
            1,
            request.Parameters.Select(ToConfiguration).ToArray(),
            request.DeviceName,
            request.DeviceNameParameterId,
            request.HostParameterId,
            request.TagIdPrefixParameterId,
            request.Enabled,
            request.Port,
            request.UnitId,
            request.PollIntervalMilliseconds,
            request.RequestTimeoutMilliseconds,
            request.Tags
                .Select(tag =>
                    new ModbusTagTemplateConfiguration(
                        tag.TagIdSuffix,
                        tag.Name,
                        tag.Address,
                        tag.Writable))
                .ToArray());
    }

    public static SnmpDeviceTemplateConfiguration ToConfiguration(
        SnmpDeviceTemplateUpsertRequest request)
    {
        ArgumentNullException.ThrowIfNull(
            request);
        ArgumentNullException.ThrowIfNull(
            request.Parameters);
        ArgumentNullException.ThrowIfNull(
            request.Tags);

        return new SnmpDeviceTemplateConfiguration(
            request.TemplateId,
            request.Name,
            1,
            request.Parameters.Select(ToConfiguration).ToArray(),
            request.DeviceName,
            request.DeviceNameParameterId,
            request.HostParameterId,
            request.CommunityParameterId,
            request.TagIdPrefixParameterId,
            request.Enabled,
            request.Port,
            request.PollIntervalMilliseconds,
            request.RequestTimeoutMilliseconds,
            request.Tags
                .Select(tag =>
                    new SnmpTagTemplateConfiguration(
                        tag.TagIdSuffix,
                        tag.Name,
                        tag.Oid))
                .ToArray());
    }

    public static ModbusDeviceConfigurationDto ToDto(
        ModbusDeviceConfiguration device)
    {
        return new ModbusDeviceConfigurationDto(
            device.DeviceId,
            device.Name,
            device.Enabled,
            device.Host,
            device.Port,
            device.UnitId,
            device.PollIntervalMilliseconds,
            device.RequestTimeoutMilliseconds,
            device.Tags
                .Select(tag =>
                    new ModbusTagConfigurationDto(
                        tag.TagId,
                        tag.Name,
                        tag.Address,
                        tag.Writable))
                .ToArray());
    }

    public static SnmpDeviceConfigurationDto ToDto(
        SnmpDeviceConfiguration device)
    {
        return new SnmpDeviceConfigurationDto(
            device.DeviceId,
            device.Name,
            device.Enabled,
            device.Host,
            device.Port,
            device.Community,
            device.PollIntervalMilliseconds,
            device.RequestTimeoutMilliseconds,
            device.Tags
                .Select(tag =>
                    new SnmpTagConfigurationDto(
                        tag.TagId,
                        tag.Name,
                        tag.Oid))
                .ToArray());
    }

    private static TemplateParameterDto ToDto(
        TemplateParameterConfiguration parameter)
    {
        return new TemplateParameterDto(
            parameter.ParameterId,
            parameter.Name);
    }

    private static TemplateParameterConfiguration ToConfiguration(
        TemplateParameterDto parameter)
    {
        ArgumentNullException.ThrowIfNull(
            parameter);

        return new TemplateParameterConfiguration(
            parameter.ParameterId,
            parameter.Name);
    }

    private static TemplateKindDto MapKind(
        TemplateKind kind)
    {
        return kind switch
        {
            TemplateKind.Mimic => TemplateKindDto.Mimic,
            TemplateKind.ModbusDevice => TemplateKindDto.ModbusDevice,
            TemplateKind.SnmpDevice => TemplateKindDto.SnmpDevice,
            _ => throw new ArgumentOutOfRangeException(nameof(kind), kind, null)
        };
    }
}
