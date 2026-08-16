namespace Dispatcher.Server.Mimics;

public static class MimicTemplateConfigurationValidator
{
    private const int MaxCanvasSize = 10000;
    private const int MaxElements = 1000;
    private const int MaxParameters = 100;

    public static void Validate(
        MimicTemplateConfiguration template)
    {
        ArgumentNullException.ThrowIfNull(
            template);
        ArgumentException.ThrowIfNullOrWhiteSpace(
            template.TemplateId);
        ArgumentException.ThrowIfNullOrWhiteSpace(
            template.Name);
        ArgumentNullException.ThrowIfNull(
            template.Parameters);
        ArgumentNullException.ThrowIfNull(
            template.Elements);

        if (template.Version < 1)
        {
            throw new InvalidOperationException(
                $"Mimic template '{template.TemplateId}' Version must be greater than zero.");
        }

        if (template.Width is < 1 or > MaxCanvasSize)
        {
            throw new InvalidOperationException(
                $"Mimic template '{template.TemplateId}' Width must be between 1 and {MaxCanvasSize}.");
        }

        if (template.Height is < 1 or > MaxCanvasSize)
        {
            throw new InvalidOperationException(
                $"Mimic template '{template.TemplateId}' Height must be between 1 and {MaxCanvasSize}.");
        }

        if (template.Parameters.Count > MaxParameters)
        {
            throw new InvalidOperationException(
                $"Mimic template '{template.TemplateId}' cannot contain more than {MaxParameters} parameters.");
        }

        if (template.Elements.Count > MaxElements)
        {
            throw new InvalidOperationException(
                $"Mimic template '{template.TemplateId}' cannot contain more than {MaxElements} elements.");
        }

        var parameterIds =
            new HashSet<string>(
                StringComparer.Ordinal);

        foreach (var parameter in template.Parameters)
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
                    $"Duplicate mimic template ParameterId '{parameter.ParameterId}'.");
            }
        }

        var elementIds =
            new HashSet<string>(
                StringComparer.Ordinal);

        foreach (var element in template.Elements)
        {
            ArgumentNullException.ThrowIfNull(
                element);
            ArgumentException.ThrowIfNullOrWhiteSpace(
                element.ElementId);

            if (!elementIds.Add(
                    element.ElementId))
            {
                throw new InvalidOperationException(
                    $"Duplicate mimic template ElementId '{element.ElementId}'.");
            }

            ValidateBounds(
                template,
                element);

            switch (element.Type)
            {
                case MimicElementType.Text:
                    ArgumentException.ThrowIfNullOrWhiteSpace(
                        element.Text);
                    EnsureNoTagBinding(
                        element);
                    break;

                case MimicElementType.Rectangle:
                    EnsureNoTagBinding(
                        element);
                    break;

                case MimicElementType.Value:
                case MimicElementType.Indicator:
                    ValidateTagBinding(
                        element,
                        parameterIds);

                    if (element.CommandValue is not null)
                    {
                        throw new InvalidOperationException(
                            $"Template element '{element.ElementId}' cannot define CommandValue for type '{element.Type}'.");
                    }

                    break;

                case MimicElementType.Button:
                    ValidateTagBinding(
                        element,
                        parameterIds);

                    if (element.CommandValue is null)
                    {
                        throw new InvalidOperationException(
                            $"Template button '{element.ElementId}' requires CommandValue.");
                    }

                    break;

                default:
                    throw new InvalidOperationException(
                        $"Unsupported mimic template element type '{element.Type}'.");
            }
        }
    }

    private static void ValidateBounds(
        MimicTemplateConfiguration template,
        MimicTemplateElementConfiguration element)
    {
        if (element.X < 0
            || element.Y < 0
            || element.Width <= 0
            || element.Height <= 0)
        {
            throw new InvalidOperationException(
                $"Template element '{element.ElementId}' must have non-negative position and positive size.");
        }

        if ((long)element.X + element.Width > template.Width
            || (long)element.Y + element.Height > template.Height)
        {
            throw new InvalidOperationException(
                $"Template element '{element.ElementId}' exceeds template bounds.");
        }
    }

    private static void ValidateTagBinding(
        MimicTemplateElementConfiguration element,
        IReadOnlySet<string> parameterIds)
    {
        var hasFixedTag =
            !string.IsNullOrWhiteSpace(
                element.TagId);
        var hasParameter =
            !string.IsNullOrWhiteSpace(
                element.TagParameterId);

        if (hasFixedTag == hasParameter)
        {
            throw new InvalidOperationException(
                $"Template element '{element.ElementId}' must define exactly one of TagId or TagParameterId.");
        }

        if (hasParameter
            && !parameterIds.Contains(
                element.TagParameterId!))
        {
            throw new InvalidOperationException(
                $"Template element '{element.ElementId}' references unknown TagParameterId '{element.TagParameterId}'.");
        }
    }

    private static void EnsureNoTagBinding(
        MimicTemplateElementConfiguration element)
    {
        if (!string.IsNullOrWhiteSpace(
                element.TagId)
            || !string.IsNullOrWhiteSpace(
                element.TagParameterId))
        {
            throw new InvalidOperationException(
                $"Template element '{element.ElementId}' of type '{element.Type}' cannot define a tag binding.");
        }

        if (element.CommandValue is not null)
        {
            throw new InvalidOperationException(
                $"Template element '{element.ElementId}' of type '{element.Type}' cannot define CommandValue.");
        }
    }
}
