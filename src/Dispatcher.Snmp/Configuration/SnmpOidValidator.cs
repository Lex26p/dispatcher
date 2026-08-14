using Lextm.SharpSnmpLib;

namespace Dispatcher.Snmp.Configuration;

public static class SnmpOidValidator
{
    public static void Validate(string oid)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(oid);
        _ = new ObjectIdentifier(oid);
    }
}
