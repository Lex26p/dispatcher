using Dispatcher.Core.Tags;
using Dispatcher.Modbus.Configuration;

namespace Dispatcher.Modbus;

public sealed class ModbusWriteService
{
    private readonly TagService _tagService;
    private readonly ModbusTcpRegisterWriter _writer;

    public ModbusWriteService(
        TagService tagService,
        ModbusTcpRegisterWriter writer)
    {
        _tagService = tagService;
        _writer = writer;
    }

    public async Task<TagValue> WriteHoldingRegisterAsync(
        ModbusTcpDevice device,
        ModbusHoldingRegisterPoint point,
        ushort value,
        TimeSpan requestTimeout,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(point);

        await _writer.WriteHoldingRegisterAsync(
            device,
            point.Address,
            value,
            requestTimeout,
            cancellationToken);

        return _tagService.Set(
            point.TagId,
            value);
    }
}
