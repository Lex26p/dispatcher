using Dispatcher.Core.Tags;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Core.Tests;

[TestClass]
public sealed class TagServiceTests
{
    [TestMethod]
    public void Set_NewTag_StoresValue()
    {
        var service = new TagService();
        var timestamp = new DateTimeOffset(2026, 8, 13, 20, 0, 0, TimeSpan.Zero);

        var stored = service.Set("pump01.pressure", 4.2, timestamp);

        Assert.AreEqual("pump01.pressure", stored.TagId);
        Assert.AreEqual(4.2, stored.Value);
        Assert.AreEqual(timestamp, stored.Timestamp);
        Assert.AreEqual(stored, service.Get("pump01.pressure"));
    }

    [TestMethod]
    public void Set_ExistingTag_ReplacesCurrentValue()
    {
        var service = new TagService();
        var firstTimestamp = new DateTimeOffset(2026, 8, 13, 20, 0, 0, TimeSpan.Zero);
        var secondTimestamp = firstTimestamp.AddSeconds(1);

        service.Set("pump01.running", false, firstTimestamp);
        var updated = service.Set("pump01.running", true, secondTimestamp);

        Assert.AreEqual(true, updated.Value);
        Assert.AreEqual(secondTimestamp, updated.Timestamp);
        Assert.AreEqual(1, service.GetAll().Count);
        Assert.AreEqual(updated, service.Get("pump01.running"));
    }

    [TestMethod]
    public void Get_UnknownTag_ReturnsNull()
    {
        var service = new TagService();

        var result = service.Get("unknown.tag");

        Assert.IsNull(result);
    }

    [TestMethod]
    public void GetAll_ReturnsCurrentValuesOrderedByTagId()
    {
        var service = new TagService();
        var timestamp = new DateTimeOffset(2026, 8, 13, 20, 0, 0, TimeSpan.Zero);

        service.Set("pump02.pressure", 2.5, timestamp);
        service.Set("pump01.pressure", 4.2, timestamp);

        var values = service.GetAll();

        Assert.AreEqual(2, values.Count);
        Assert.AreEqual("pump01.pressure", values[0].TagId);
        Assert.AreEqual("pump02.pressure", values[1].TagId);
    }

    [TestMethod]
    public void Set_EmptyTagId_ThrowsArgumentException()
    {
        var service = new TagService();

        Assert.Throws<ArgumentException>(() => service.Set(" ", 42));
    }

    [TestMethod]
    public void Get_EmptyTagId_ThrowsArgumentException()
    {
        var service = new TagService();

        Assert.Throws<ArgumentException>(() => service.Get(""));
    }
}
