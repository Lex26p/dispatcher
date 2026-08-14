using Lextm.SharpSnmpLib;

namespace Dispatcher.Snmp;

public static class SnmpValueConverter
{
    public static object? ToRuntimeValue(ISnmpData data)
    {
        ArgumentNullException.ThrowIfNull(data);

        if (data.TypeCode is
            SnmpType.NoSuchObject or
            SnmpType.NoSuchInstance or
            SnmpType.EndOfMibView)
        {
            throw new InvalidOperationException(
                $"SNMP response contains {data.TypeCode}.");
        }

        return data switch
        {
            Integer32 value => value.ToInt32(),
            Counter32 value => value.ToUInt32(),
            Gauge32 value => value.ToUInt32(),
            TimeTicks value => value.ToUInt32(),
            Counter64 value => value.ToUInt64(),
            OctetString value => value.ToString(),
            Null _ => null,
            _ => data.ToString()
        };
    }
}
