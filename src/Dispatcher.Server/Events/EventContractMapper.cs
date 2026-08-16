using Dispatcher.Contracts.Events;

namespace Dispatcher.Server.Events;

internal static class EventContractMapper
{
    public static EventRecordDto ToDto(
        EventRecord record)
    {
        ArgumentNullException.ThrowIfNull(
            record);

        return new EventRecordDto(
            record.EventId,
            record.Timestamp,
            ToDto(
                record.Category),
            record.Type,
            ToDto(
                record.Severity),
            record.Source,
            record.Message,
            record.DataJson,
            record.ActorUserId,
            record.ActorUserName);
    }

    public static EventCategory ToInternal(
        EventCategoryDto category)
    {
        return category switch
        {
            EventCategoryDto.System =>
                EventCategory.System,
            EventCategoryDto.Device =>
                EventCategory.Device,
            EventCategoryDto.Command =>
                EventCategory.Command,
            EventCategoryDto.Configuration =>
                EventCategory.Configuration,
            _ =>
                throw new ArgumentOutOfRangeException(
                    nameof(category),
                    category,
                    null)
        };
    }

    public static EventSeverity ToInternal(
        EventSeverityDto severity)
    {
        return severity switch
        {
            EventSeverityDto.Information =>
                EventSeverity.Information,
            EventSeverityDto.Warning =>
                EventSeverity.Warning,
            EventSeverityDto.Error =>
                EventSeverity.Error,
            _ =>
                throw new ArgumentOutOfRangeException(
                    nameof(severity),
                    severity,
                    null)
        };
    }

    private static EventCategoryDto ToDto(
        EventCategory category)
    {
        return category switch
        {
            EventCategory.System =>
                EventCategoryDto.System,
            EventCategory.Device =>
                EventCategoryDto.Device,
            EventCategory.Command =>
                EventCategoryDto.Command,
            EventCategory.Configuration =>
                EventCategoryDto.Configuration,
            _ =>
                throw new ArgumentOutOfRangeException(
                    nameof(category),
                    category,
                    null)
        };
    }

    private static EventSeverityDto ToDto(
        EventSeverity severity)
    {
        return severity switch
        {
            EventSeverity.Information =>
                EventSeverityDto.Information,
            EventSeverity.Warning =>
                EventSeverityDto.Warning,
            EventSeverity.Error =>
                EventSeverityDto.Error,
            _ =>
                throw new ArgumentOutOfRangeException(
                    nameof(severity),
                    severity,
                    null)
        };
    }
}
