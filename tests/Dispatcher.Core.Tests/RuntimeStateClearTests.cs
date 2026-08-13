using Dispatcher.Core.Devices;
using Dispatcher.Core.Tags;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Core.Tests;

[TestClass]
public sealed class RuntimeStateClearTests
{
    [TestMethod]
    public void TagService_Clear_RemovesCurrentValues()
    {
        var service = new TagService();

        service.Set("device01.register100", 1234);

        service.Clear();

        Assert.AreEqual(0, service.GetAll().Count);
    }

    [TestMethod]
    public void DeviceStateService_Clear_RemovesCurrentStates()
    {
        var service = new DeviceStateService();

        service.SetOnline(
            "device01",
            DateTimeOffset.UtcNow);

        service.Clear();

        Assert.AreEqual(0, service.GetAll().Count);
    }
}
