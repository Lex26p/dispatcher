using System.Net;
using Dispatcher.Core.Tags;
using Dispatcher.Modbus.Configuration;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Modbus.Tests;

[TestClass]
public sealed class ModbusTcpReadTests
{
    [TestMethod]
    public async Task ReadHoldingRegister_FromTcpServer_StoresTagValue()
    {
        using var server = new TestModbusTcpServer(
            expectedUnitId: 1,
            valueFactory: (_, _, address) =>
            {
                Assert.AreEqual((ushort)100, address);
                return 1234;
            });

        var serverTask = server.ServeAsync(1);

        var tagService = new TagService();
        var reader = new ModbusTcpRegisterReader();
        var readService = new ModbusReadService(tagService, reader);

        var device = new ModbusTcpDevice(
            DeviceId: "device01",
            Host: IPAddress.Loopback.ToString(),
            Port: server.Port,
            UnitId: 1);

        var point = new ModbusHoldingRegisterPoint(
            TagId: "device01.register100",
            Address: 100);

        var result = readService.ReadHoldingRegister(device, point);

        await serverTask;

        Assert.AreEqual("device01.register100", result.TagId);
        Assert.AreEqual((ushort)1234, result.Value);
        Assert.AreEqual(result, tagService.Get("device01.register100"));
    }
}
