namespace Dispatcher.Server.Mimics;

public static class MimicConfigurationValidator
{
    private const int MaxCanvasSize = 10000;
    private const int MaxElements = 1000;

    public static void Validate(MimicConfiguration mimic)
    {
        ArgumentNullException.ThrowIfNull(mimic);
        ArgumentException.ThrowIfNullOrWhiteSpace(mimic.MimicId);
        ArgumentException.ThrowIfNullOrWhiteSpace(mimic.Name);
        ArgumentNullException.ThrowIfNull(mimic.Elements);

        if (mimic.Width is < 1 or > MaxCanvasSize)
        {
            throw new InvalidOperationException(
                $"Mimic '{mimic.MimicId}' Width must be between 1 and {MaxCanvasSize}.");
        }

        if (mimic.Height is < 1 or > MaxCanvasSize)
        {
            throw new InvalidOperationException(
                $"Mimic '{mimic.MimicId}' Height must be between 1 and {MaxCanvasSize}.");
        }

        if (mimic.Elements.Count > MaxElements)
        {
            throw new InvalidOperationException(
                $"Mimic '{mimic.MimicId}' cannot contain more than {MaxElements} elements.");
        }

        var elementIds = new HashSet<string>(StringComparer.Ordinal);

        foreach (var element in mimic.Elements)
        {
            ArgumentNullException.ThrowIfNull(element);
            ArgumentException.ThrowIfNullOrWhiteSpace(element.ElementId);

            if (!elementIds.Add(element.ElementId))
            {
                throw new InvalidOperationException(
                    $"Duplicate mimic ElementId '{element.ElementId}'.");
            }

            ValidateBounds(
                mimic,
                element);

            switch (element.Type)
            {
                case MimicElementType.Text:
                    ArgumentException.ThrowIfNullOrWhiteSpace(element.Text);
                    break;

                case MimicElementType.Rectangle:
                    break;

                case MimicElementType.Value:
                case MimicElementType.Indicator:
                    ArgumentException.ThrowIfNullOrWhiteSpace(element.TagId);
                    break;

                case MimicElementType.Button:
                    ArgumentException.ThrowIfNullOrWhiteSpace(element.TagId);

                    if (element.CommandValue is null)
                    {
                        throw new InvalidOperationException(
                            $"Button '{element.ElementId}' requires CommandValue.");
                    }

                    break;

                default:
                    throw new InvalidOperationException(
                        $"Unsupported mimic element type '{element.Type}'.");
            }
        }
    }

    private static void ValidateBounds(
        MimicConfiguration mimic,
        MimicElementConfiguration element)
    {
        if (element.X < 0
            || element.Y < 0
            || element.Width <= 0
            || element.Height <= 0)
        {
            throw new InvalidOperationException(
                $"Element '{element.ElementId}' must have non-negative position and positive size.");
        }

        if ((long)element.X + element.Width > mimic.Width
            || (long)element.Y + element.Height > mimic.Height)
        {
            throw new InvalidOperationException(
                $"Element '{element.ElementId}' exceeds mimic canvas bounds.");
        }
    }
}
