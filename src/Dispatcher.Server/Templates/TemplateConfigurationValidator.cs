namespace Dispatcher.Server.Templates;

public static class TemplateConfigurationValidator
{
    public const int MaxParameters = 100;

    public static void ValidateCatalogEntry(
        TemplateCatalogEntryConfiguration entry)
    {
        ArgumentNullException.ThrowIfNull(
            entry);
        ArgumentException.ThrowIfNullOrWhiteSpace(
            entry.TemplateId);
        ArgumentException.ThrowIfNullOrWhiteSpace(
            entry.Name);
        ArgumentNullException.ThrowIfNull(
            entry.Parameters);

        if (!Enum.IsDefined(
                typeof(TemplateKind),
                entry.Kind))
        {
            throw new InvalidOperationException(
                $"Unknown template kind '{entry.Kind}'.");
        }

        if (entry.Version < 1)
        {
            throw new InvalidOperationException(
                $"Template '{entry.TemplateId}' Version must be greater than zero.");
        }

        ValidateParameters(
            entry.TemplateId,
            entry.Parameters);
    }

    public static IReadOnlySet<string> ValidateParameters(
        string templateId,
        IReadOnlyList<TemplateParameterConfiguration> parameters)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            templateId);
        ArgumentNullException.ThrowIfNull(
            parameters);

        if (parameters.Count > MaxParameters)
        {
            throw new InvalidOperationException(
                $"Template '{templateId}' cannot contain more than {MaxParameters} parameters.");
        }

        var parameterIds =
            new HashSet<string>(
                StringComparer.Ordinal);

        foreach (var parameter in parameters)
        {
            ArgumentNullException.ThrowIfNull(
                parameter);
            ArgumentException.ThrowIfNullOrWhiteSpace(
                parameter.ParameterId);
            ArgumentException.ThrowIfNullOrWhiteSpace(
                parameter.Name);

            if (!parameterIds.Add(
                    parameter.ParameterId))
            {
                throw new InvalidOperationException(
                    $"Duplicate template ParameterId '{parameter.ParameterId}'.");
            }
        }

        return parameterIds;
    }

    public static void ValidateReferencedParameter(
        string templateId,
        string fieldName,
        string? parameterId,
        IReadOnlySet<string> parameterIds,
        bool required)
    {
        if (string.IsNullOrWhiteSpace(
                parameterId))
        {
            if (required)
            {
                throw new InvalidOperationException(
                    $"Template '{templateId}' {fieldName} is required.");
            }

            return;
        }

        if (!parameterIds.Contains(
                parameterId))
        {
            throw new InvalidOperationException(
                $"Template '{templateId}' {fieldName} references unknown parameter '{parameterId}'.");
        }
    }

    public static IReadOnlyDictionary<string, string> ValidateInstanceParameters(
        TemplateCatalogEntryConfiguration entry,
        IReadOnlyDictionary<string, string> values)
    {
        ArgumentNullException.ThrowIfNull(
            values);

        var parameterIds =
            entry.Parameters
                .Select(parameter => parameter.ParameterId)
                .ToHashSet(
                    StringComparer.Ordinal);

        foreach (var key in values.Keys)
        {
            if (!parameterIds.Contains(
                    key))
            {
                throw new InvalidOperationException(
                    $"Unknown template parameter '{key}'.");
            }
        }

        var normalized =
            new Dictionary<string, string>(
                StringComparer.Ordinal);

        foreach (var parameter in entry.Parameters)
        {
            if (!values.TryGetValue(
                    parameter.ParameterId,
                    out var value)
                || string.IsNullOrWhiteSpace(
                    value))
            {
                throw new InvalidOperationException(
                    $"Template parameter '{parameter.ParameterId}' requires a value.");
            }

            normalized[parameter.ParameterId] =
                value.Trim();
        }

        return normalized;
    }
}
