using Dispatcher.Server.Configuration;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Server.Tests;

[TestClass]
public sealed class ConfigurationCatalogTests
{
    [TestMethod]
    public void ReplaceAll_DuplicateTagIdAcrossProtocols_Throws()
    {
        var catalog =
            new ConfigurationCatalog();

        var modbus =
            new ModbusDeviceConfiguration(
                "modbus01",
                "Modbus",
                false,
                "127.0.0.1",
                502,
                1,
                1000,
                1000,
                [
                    new ModbusTagConfiguration(
                        "shared.tag",
                        "Shared",
                        0,
                        false)
                ]);

        var snmp =
            new SnmpDeviceConfiguration(
                "snmp01",
                "SNMP",
                false,
                "127.0.0.1",
                161,
                "public",
                1000,
                1000,
                [
                    new SnmpTagConfiguration(
                        "shared.tag",
                        "Shared",
                        "1.3.6.1.2.1.1.5.0")
                ]);

        try
        {
            catalog.ReplaceAll(
                [modbus],
                [snmp]);

            Assert.Fail(
                "Cross-protocol duplicate TagId must be rejected.");
        }
        catch (InvalidOperationException)
        {
        }
    }
}
