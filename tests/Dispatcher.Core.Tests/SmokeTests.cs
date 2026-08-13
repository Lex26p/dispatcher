using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Core.Tests;

[TestClass]
public sealed class SmokeTests
{
    [TestMethod]
    public void TestInfrastructure_IsOperational()
    {
        Assert.AreEqual(4, 2 + 2);
    }
}
