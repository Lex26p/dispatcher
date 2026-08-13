using Dispatcher.Core.Tags;
using Dispatcher.Modbus.Configuration;

namespace Dispatcher.Modbus;

public sealed class ModbusReadService
{
    private readonly TagService _tagService;
    private readonly ModbusTcpRegisterReader _reader;

    public ModbusReadService(
        TagService tagService,
        ModbusTcpRegisterReader reader)
    {
        _tagService = tagService;
        _reader = reader;
    }

    public TagValue ReadHoldingRegister(
        ModbusTcpDevice device,
        ModbusHoldingRegisterPoint point)
    {
        ArgumentNullException.ThrowIfNull(point);

        var value = _reader.ReadHoldingRegister(device, point.Address);

        return _tagService.Set(point.TagId, value);
    }
}
