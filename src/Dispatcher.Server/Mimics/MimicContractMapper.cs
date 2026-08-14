using Dispatcher.Contracts.Mimics;

namespace Dispatcher.Server.Mimics;

internal static class MimicContractMapper
{
    public static MimicDefinitionDto ToDto(
        MimicConfiguration mimic)
    {
        return new MimicDefinitionDto(
            mimic.MimicId,
            mimic.Name,
            mimic.Width,
            mimic.Height,
            mimic.Elements
                .Select(ToDto)
                .ToArray());
    }

    public static MimicSummaryDto ToSummaryDto(
        MimicConfiguration mimic)
    {
        return new MimicSummaryDto(
            mimic.MimicId,
            mimic.Name,
            mimic.Width,
            mimic.Height,
            mimic.Elements.Count);
    }

    public static MimicConfiguration ToConfiguration(
        MimicDefinitionDto mimic)
    {
        ArgumentNullException.ThrowIfNull(mimic);
        ArgumentNullException.ThrowIfNull(mimic.Elements);

        return new MimicConfiguration(
            mimic.MimicId,
            mimic.Name,
            mimic.Width,
            mimic.Height,
            mimic.Elements
                .Select(ToConfiguration)
                .ToArray());
    }

    private static MimicElementDto ToDto(
        MimicElementConfiguration element)
    {
        return new MimicElementDto(
            element.ElementId,
            MapType(element.Type),
            element.X,
            element.Y,
            element.Width,
            element.Height,
            element.Text,
            element.TagId,
            element.CommandValue);
    }

    private static MimicElementConfiguration ToConfiguration(
        MimicElementDto element)
    {
        ArgumentNullException.ThrowIfNull(element);

        return new MimicElementConfiguration(
            element.ElementId,
            MapType(element.Type),
            element.X,
            element.Y,
            element.Width,
            element.Height,
            element.Text,
            element.TagId,
            element.CommandValue);
    }

    private static MimicElementTypeDto MapType(
        MimicElementType type)
    {
        return type switch
        {
            MimicElementType.Text => MimicElementTypeDto.Text,
            MimicElementType.Rectangle => MimicElementTypeDto.Rectangle,
            MimicElementType.Value => MimicElementTypeDto.Value,
            MimicElementType.Indicator => MimicElementTypeDto.Indicator,
            MimicElementType.Button => MimicElementTypeDto.Button,
            _ => throw new ArgumentOutOfRangeException(nameof(type), type, null)
        };
    }

    private static MimicElementType MapType(
        MimicElementTypeDto type)
    {
        return type switch
        {
            MimicElementTypeDto.Text => MimicElementType.Text,
            MimicElementTypeDto.Rectangle => MimicElementType.Rectangle,
            MimicElementTypeDto.Value => MimicElementType.Value,
            MimicElementTypeDto.Indicator => MimicElementType.Indicator,
            MimicElementTypeDto.Button => MimicElementType.Button,
            _ => throw new ArgumentOutOfRangeException(nameof(type), type, null)
        };
    }
}
