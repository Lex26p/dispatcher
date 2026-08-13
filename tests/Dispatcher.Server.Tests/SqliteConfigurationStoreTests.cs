using Dispatcher.Server.Configuration;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Server.Tests;

[TestClass]
public sealed class SqliteConfigurationStoreTests
{
    [TestMethod]
    public async Task ReplaceAsync_PersistsDevicesAndTags_AcrossStoreInstances()
    {
        var device =
            new ModbusDeviceConfiguration(
                DeviceId: "plc01",
                Name: "PLC 01",
                Enabled: true,
                Host: "192.168.1.10",
                Port: 1502,
                UnitId: 7,
                PollIntervalMilliseconds: 250,
                RequestTimeoutMilliseconds: 900,
                Tags:
                [
                    new ModbusTagConfiguration(
                        TagId: "plc01.temperature",
                        Name: "Temperature",
                        Address: 100,
                        Writable: false),
                    new ModbusTagConfiguration(
                        TagId: "plc01.setpoint",
                        Name: "Setpoint",
                        Address: 101,
                        Writable: true)
                ]);

        using var database =
            await TestConfigurationDatabase.CreateAsync(
                device);

        var reopenedStore =
            new SqliteConfigurationStore(
                database.DatabasePath);

        await reopenedStore.InitializeAsync();

        var loaded =
            await reopenedStore.LoadAsync();

        Assert.AreEqual(1, loaded.Count);

        var loadedDevice = loaded[0];

        Assert.AreEqual("plc01", loadedDevice.DeviceId);
        Assert.AreEqual("PLC 01", loadedDevice.Name);
        Assert.IsTrue(loadedDevice.Enabled);
        Assert.AreEqual("192.168.1.10", loadedDevice.Host);
        Assert.AreEqual(1502, loadedDevice.Port);
        Assert.AreEqual(7, loadedDevice.UnitId);
        Assert.AreEqual(250, loadedDevice.PollIntervalMilliseconds);
        Assert.AreEqual(900, loadedDevice.RequestTimeoutMilliseconds);

        Assert.AreEqual(2, loadedDevice.Tags.Count);

        var setpoint =
            loadedDevice.Tags.Single(
                tag => tag.TagId == "plc01.setpoint");

        Assert.AreEqual("Setpoint", setpoint.Name);
        Assert.AreEqual(101, setpoint.Address);
        Assert.IsTrue(setpoint.Writable);
    }
}
