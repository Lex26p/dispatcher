using Dispatcher.Contracts.Mimics;

namespace Dispatcher.Server.Mimics;

internal static class MimicTemplateContractMapper
{
    public static MimicTemplateDto ToDto(
        MimicTemplateConfiguration template)
    {
        return new MimicTemplateDto(
            template.TemplateId,
            template.Name,
            template.Width,
            template.Height,
            template.Parameters
                .Select(parameter =>
                    new MimicTemplateParameterDto(
                        parameter.ParameterId,
                        parameter.Name))
                .ToArray(),
            template.Elements
                .Select(ToDto)
                .ToArray());
    }

    public static MimicTemplateConfiguration ToConfiguration(
        MimicTemplateDto template)
    {
        ArgumentNullException.ThrowIfNull(
            template);
        ArgumentNullException.ThrowIfNull(
            template.Parameters);
        ArgumentNullException.ThrowIfNull(
            template.Elements);

        return new MimicTemplateConfiguration(
            template.TemplateId,
            template.Name,
            template.Width,
            template.Height,
            template.Parameters
                .Select(
                    ToConfiguration)
                .ToArray(),
            template.Elements
                .Select(ToConfiguration)
                .ToArray());
    }

    private static MimicTemplateParameterConfiguration ToConfiguration(
        MimicTemplateParameterDto parameter)
    {
        ArgumentNullException.ThrowIfNull(
            parameter);

        return new MimicTemplateParameterConfiguration(
            parameter.ParameterId,
            parameter.Name);
    }

    private static MimicTemplateElementDto ToDto(
        MimicTemplateElementConfiguration element)
    {
        return new MimicTemplateElementDto(
            element.ElementId,
            MapType(
                element.Type),
            element.X,
            element.Y,
            element.Width,
            element.Height,
            element.Text,
            element.TagId,
            element.TagParameterId,
            element.CommandValue);
    }

    private static MimicTemplateElementConfiguration ToConfiguration(
        MimicTemplateElementDto element)
    {
        ArgumentNullException.ThrowIfNull(
            element);

        return new MimicTemplateElementConfiguration(
            element.ElementId,
            MapType(
                element.Type),
            element.X,
            element.Y,
            element.Width,
            element.Height,
            element.Text,
            element.TagId,
            element.TagParameterId,
            element.CommandValue);
    }

    private static MimicElementTypeDto MapType(
        MimicElementType type)
    {
        return type switch
        {
            MimicElementType.Text =>
                MimicElementTypeDto.Text,
            MimicElementType.Rectangle =>
                MimicElementTypeDto.Rectangle,
            MimicElementType.Value =>
                MimicElementTypeDto.Value,
            MimicElementType.Indicator =>
                MimicElementTypeDto.Indicator,
            MimicElementType.Button =>
                MimicElementTypeDto.Button,
            _ =>
                throw new ArgumentOutOfRangeException(
                    nameof(type),
                    type,
                    null)
        };
    }

    private static MimicElementType MapType(
        MimicElementTypeDto type)
    {
        return type switch
        {
            MimicElementTypeDto.Text =>
                MimicElementType.Text,
            MimicElementTypeDto.Rectangle =>
                MimicElementType.Rectangle,
            MimicElementTypeDto.Value =>
                MimicElementType.Value,
            MimicElementTypeDto.Indicator =>
                MimicElementType.Indicator,
            MimicElementTypeDto.Button =>
                MimicElementType.Button,
            _ =>
                throw new ArgumentOutOfRangeException(
                    nameof(type),
                    type,
                    null)
        };
    }
}
