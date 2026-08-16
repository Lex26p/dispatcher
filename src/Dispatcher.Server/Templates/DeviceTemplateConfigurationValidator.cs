using Dispatcher.Snmp.Configuration;

namespace Dispatcher.Server.Templates;

public static class DeviceTemplateConfigurationValidator
{
    private const int MaxTags = 1000;

    public static void Validate(
        ModbusDeviceTemplateConfiguration template)
    {
        ArgumentNullException.ThrowIfNull(
            template);
        ValidateCommon(
            template.TemplateId,
            template.Name,
            template.Version,
            template.Parameters,
            template.DeviceName,
            template.DeviceNameParameterId,
            template.HostParameterId,
            template.TagIdPrefixParameterId,
            template.Port,
            template.PollIntervalMilliseconds,
            template.RequestTimeoutMilliseconds);
        ArgumentNullException.ThrowIfNull(
            template.Tags);

        if (template.UnitId is < 0 or > byte.MaxValue)
        {
            throw new InvalidOperationException(
                $"Modbus template '{template.TemplateId}' UnitId must be between 0 and 255.");
        }

        if (template.Tags.Count > MaxTags)
        {
            throw new InvalidOperationException(
                $"Modbus template '{template.TemplateId}' cannot contain more than {MaxTags} tags.");
        }

        var suffixes =
            new HashSet<string>(
                StringComparer.Ordinal);

        foreach (var tag in template.Tags)
        {
            ArgumentNullException.ThrowIfNull(
                tag);
            ArgumentException.ThrowIfNullOrWhiteSpace(
                tag.TagIdSuffix);
            ArgumentException.ThrowIfNullOrWhiteSpace(
                tag.Name);

            if (!suffixes.Add(
                    tag.TagIdSuffix))
            {
                throw new InvalidOperationException(
                    $"Duplicate Modbus template TagIdSuffix '{tag.TagIdSuffix}'.");
            }

            if (tag.Address is < 0 or > ushort.MaxValue)
            {
                throw new InvalidOperationException(
                    $"Modbus template tag '{tag.TagIdSuffix}' Address must be between 0 and 65535.");
            }
        }
    }

    public static void Validate(
        SnmpDeviceTemplateConfiguration template)
    {
        ArgumentNullException.ThrowIfNull(
            template);
        var parameterIds =
            ValidateCommon(
                template.TemplateId,
                template.Name,
                template.Version,
                template.Parameters,
                template.DeviceName,
                template.DeviceNameParameterId,
                template.HostParameterId,
                template.TagIdPrefixParameterId,
                template.Port,
                template.PollIntervalMilliseconds,
                template.RequestTimeoutMilliseconds);

        TemplateConfigurationValidator.ValidateReferencedParameter(
            template.TemplateId,
            nameof(template.CommunityParameterId),
            template.CommunityParameterId,
            parameterIds,
            required: true);

        ArgumentNullException.ThrowIfNull(
            template.Tags);

        if (template.Tags.Count > MaxTags)
        {
            throw new InvalidOperationException(
                $"SNMP template '{template.TemplateId}' cannot contain more than {MaxTags} tags.");
        }

        var suffixes =
            new HashSet<string>(
                StringComparer.Ordinal);

        foreach (var tag in template.Tags)
        {
            ArgumentNullException.ThrowIfNull(
                tag);
            ArgumentException.ThrowIfNullOrWhiteSpace(
                tag.TagIdSuffix);
            ArgumentException.ThrowIfNullOrWhiteSpace(
                tag.Name);
            ArgumentException.ThrowIfNullOrWhiteSpace(
                tag.Oid);

            if (!suffixes.Add(
                    tag.TagIdSuffix))
            {
                throw new InvalidOperationException(
                    $"Duplicate SNMP template TagIdSuffix '{tag.TagIdSuffix}'.");
            }

            try
            {
                SnmpOidValidator.Validate(
                    tag.Oid);
            }
            catch (Exception exception)
                when (exception is ArgumentException or FormatException or OverflowException)
            {
                throw new InvalidOperationException(
                    $"SNMP template tag '{tag.TagIdSuffix}' contains invalid OID '{tag.Oid}'.",
                    exception);
            }
        }
    }

    private static IReadOnlySet<string> ValidateCommon(
        string templateId,
        string name,
        int version,
        IReadOnlyList<TemplateParameterConfiguration> parameters,
        string deviceName,
        string? deviceNameParameterId,
        string hostParameterId,
        string tagIdPrefixParameterId,
        int port,
        int pollIntervalMilliseconds,
        int requestTimeoutMilliseconds)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            templateId);
        ArgumentException.ThrowIfNullOrWhiteSpace(
            name);
        ArgumentException.ThrowIfNullOrWhiteSpace(
            deviceName);

        var parameterIds =
            TemplateConfigurationValidator.ValidateParameters(
                templateId,
                parameters);

        TemplateConfigurationValidator.ValidateReferencedParameter(
            templateId,
            nameof(deviceNameParameterId),
            deviceNameParameterId,
            parameterIds,
            required: false);
        TemplateConfigurationValidator.ValidateReferencedParameter(
            templateId,
            nameof(hostParameterId),
            hostParameterId,
            parameterIds,
            required: true);
        TemplateConfigurationValidator.ValidateReferencedParameter(
            templateId,
            nameof(tagIdPrefixParameterId),
            tagIdPrefixParameterId,
            parameterIds,
            required: true);

        if (version < 1)
        {
            throw new InvalidOperationException(
                $"Template '{templateId}' Version must be greater than zero.");
        }

        if (port is < 1 or > 65535)
        {
            throw new InvalidOperationException(
                $"Template '{templateId}' Port must be between 1 and 65535.");
        }

        if (pollIntervalMilliseconds <= 0)
        {
            throw new InvalidOperationException(
                $"Template '{templateId}' PollIntervalMilliseconds must be greater than zero.");
        }

        if (requestTimeoutMilliseconds <= 0)
        {
            throw new InvalidOperationException(
                $"Template '{templateId}' RequestTimeoutMilliseconds must be greater than zero.");
        }

        return parameterIds;
    }
}
