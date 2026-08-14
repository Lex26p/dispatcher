namespace Dispatcher.Snmp.Configuration;

public sealed record SnmpPollingPlan(
    SnmpV2cDevice Device,
    IReadOnlyList<SnmpPoint> Points,
    TimeSpan PollInterval,
    TimeSpan RequestTimeout);
