using System.Buffers.Binary;
using System.Net;
using System.Net.Sockets;
using System.Text;

namespace Dispatcher.Tests.Shared;

internal sealed class SnmpV2cTestAgent : IDisposable
{
    private readonly UdpClient _udp;
    private readonly string _expectedCommunity;
    private readonly byte[] _expectedOid;

    public SnmpV2cTestAgent(
        string expectedCommunity,
        string expectedOid)
    {
        _expectedCommunity =
            expectedCommunity;
        _expectedOid =
            EncodeOidContent(
                expectedOid);

        _udp =
            new UdpClient(
                new IPEndPoint(
                    IPAddress.Loopback,
                    0));

        Port =
            ((IPEndPoint)_udp.Client.LocalEndPoint!).Port;
    }

    public int Port { get; }

    public Task ServeInteger32OnceAsync(
        int value)
    {
        return ServeOnceAsync(
            0x02,
            EncodeIntegerContent(
                value));
    }

    public Task ServeOctetStringOnceAsync(
        string value)
    {
        return ServeOctetStringAsync(
            value,
            count: 1);
    }

    public Task ServeOctetStringAsync(
        string value,
        int count)
    {
        if (count <= 0)
        {
            throw new ArgumentOutOfRangeException(
                nameof(count));
        }

        return ServeAsync(
            0x04,
            Encoding.UTF8.GetBytes(
                value),
            count);
    }

    private Task ServeOnceAsync(
        byte responseDataTag,
        byte[] responseData)
    {
        return ServeAsync(
            responseDataTag,
            responseData,
            count: 1);
    }

    private async Task ServeAsync(
        byte responseDataTag,
        byte[] responseData,
        int count)
    {
        for (var index = 0; index < count; index++)
        {
            var received =
                await _udp.ReceiveAsync();

            var request =
                ParseRequest(
                    received.Buffer);

            if (!string.Equals(
                    _expectedCommunity,
                    request.Community,
                    StringComparison.Ordinal))
            {
                throw new InvalidOperationException(
                    $"Unexpected SNMP community '{request.Community}'.");
            }

            if (!request.Oid.AsSpan()
                    .SequenceEqual(
                        _expectedOid))
            {
                throw new InvalidOperationException(
                    "Unexpected SNMP OID.");
            }

            var response =
                BuildResponse(
                    request,
                    responseDataTag,
                    responseData);

            await _udp.SendAsync(
                response,
                response.Length,
                received.RemoteEndPoint);
        }
    }

    private static SnmpRequest ParseRequest(
        byte[] bytes)
    {
        var root =
            new BerReader(
                bytes)
                .ReadExpected(0x30);

        var message =
            new BerReader(
                root.Content);

        var version =
            DecodeInteger(
                message
                    .ReadExpected(0x02)
                    .Content);

        if (version != 1)
        {
            throw new InvalidOperationException(
                $"Expected SNMP v2c version integer 1, got {version}.");
        }

        var community =
            Encoding.ASCII.GetString(
                message
                    .ReadExpected(0x04)
                    .Content);

        var pdu =
            new BerReader(
                message
                    .ReadExpected(0xA0)
                    .Content);

        var requestId =
            pdu
                .ReadExpected(0x02)
                .Content;

        _ = pdu.ReadExpected(0x02);
        _ = pdu.ReadExpected(0x02);

        var varBindList =
            new BerReader(
                pdu
                    .ReadExpected(0x30)
                    .Content);

        var varBind =
            new BerReader(
                varBindList
                    .ReadExpected(0x30)
                    .Content);

        var oid =
            varBind
                .ReadExpected(0x06)
                .Content;

        _ = varBind.ReadExpected(0x05);

        return new SnmpRequest(
            community,
            requestId,
            oid);
    }

    private static byte[] BuildResponse(
        SnmpRequest request,
        byte dataTag,
        byte[] data)
    {
        var variable =
            EncodeTlv(
                0x30,
                Combine(
                    EncodeTlv(
                        0x06,
                        request.Oid),
                    EncodeTlv(
                        dataTag,
                        data)));

        var variableList =
            EncodeTlv(
                0x30,
                variable);

        var responsePdu =
            EncodeTlv(
                0xA2,
                Combine(
                    EncodeTlv(
                        0x02,
                        request.RequestId),
                    EncodeTlv(
                        0x02,
                        [0]),
                    EncodeTlv(
                        0x02,
                        [0]),
                    variableList));

        return EncodeTlv(
            0x30,
            Combine(
                EncodeTlv(
                    0x02,
                    [1]),
                EncodeTlv(
                    0x04,
                    Encoding.ASCII.GetBytes(
                        request.Community)),
                responsePdu));
    }

    private static int DecodeInteger(
        byte[] bytes)
    {
        if (bytes.Length is < 1 or > 4)
        {
            throw new InvalidOperationException(
                "Unsupported BER integer length.");
        }

        var value =
            (bytes[0] & 0x80) == 0
                ? 0
                : -1;

        foreach (var item in bytes)
        {
            value =
                (value << 8)
                | item;
        }

        return value;
    }

    private static byte[] EncodeIntegerContent(
        int value)
    {
        Span<byte> buffer =
            stackalloc byte[4];

        BinaryPrimitives.WriteInt32BigEndian(
            buffer,
            value);

        var start = 0;

        while (start < 3)
        {
            var current =
                buffer[start];
            var next =
                buffer[start + 1];

            if (current == 0x00
                && (next & 0x80) == 0)
            {
                start++;
                continue;
            }

            if (current == 0xFF
                && (next & 0x80) != 0)
            {
                start++;
                continue;
            }

            break;
        }

        return buffer[start..]
            .ToArray();
    }

    private static byte[] EncodeOidContent(
        string oid)
    {
        var parts =
            oid.Split(
                    '.',
                    StringSplitOptions.RemoveEmptyEntries)
                .Select(uint.Parse)
                .ToArray();

        if (parts.Length < 2)
        {
            throw new ArgumentException(
                "OID must contain at least two arcs.",
                nameof(oid));
        }

        var result =
            new List<byte>();

        AppendBase128(
            result,
            checked(
                parts[0] * 40
                + parts[1]));

        foreach (var part in parts.Skip(2))
        {
            AppendBase128(
                result,
                part);
        }

        return result.ToArray();
    }

    private static void AppendBase128(
        List<byte> result,
        uint value)
    {
        Span<byte> buffer =
            stackalloc byte[5];

        var index =
            buffer.Length;

        buffer[--index] =
            (byte)(value & 0x7F);

        value >>= 7;

        while (value != 0)
        {
            buffer[--index] =
                (byte)(
                    (value & 0x7F)
                    | 0x80);

            value >>= 7;
        }

        for (; index < buffer.Length; index++)
        {
            result.Add(
                buffer[index]);
        }
    }

    private static byte[] EncodeTlv(
        byte tag,
        byte[] content)
    {
        return Combine(
            [tag],
            EncodeLength(
                content.Length),
            content);
    }

    private static byte[] EncodeLength(
        int length)
    {
        if (length < 0x80)
        {
            return [(byte)length];
        }

        Span<byte> buffer =
            stackalloc byte[4];

        BinaryPrimitives.WriteInt32BigEndian(
            buffer,
            length);

        var start = 0;

        while (start < 3
               && buffer[start] == 0)
        {
            start++;
        }

        var count =
            buffer.Length - start;

        var result =
            new byte[count + 1];

        result[0] =
            (byte)(0x80 | count);

        buffer[start..]
            .CopyTo(
                result.AsSpan(1));

        return result;
    }

    private static byte[] Combine(
        params byte[][] arrays)
    {
        var length =
            arrays.Sum(
                array => array.Length);

        var result =
            new byte[length];

        var offset = 0;

        foreach (var array in arrays)
        {
            array.CopyTo(
                result,
                offset);

            offset +=
                array.Length;
        }

        return result;
    }

    public void Dispose()
    {
        _udp.Dispose();
    }

    private sealed record SnmpRequest(
        string Community,
        byte[] RequestId,
        byte[] Oid);

    private sealed class BerReader
    {
        private readonly byte[] _buffer;
        private int _offset;

        public BerReader(
            byte[] buffer)
        {
            _buffer =
                buffer;
        }

        public Tlv ReadExpected(
            byte expectedTag)
        {
            var value =
                Read();

            if (value.Tag != expectedTag)
            {
                throw new InvalidOperationException(
                    $"Expected BER tag 0x{expectedTag:X2}, got 0x{value.Tag:X2}.");
            }

            return value;
        }

        private Tlv Read()
        {
            if (_offset >= _buffer.Length)
            {
                throw new InvalidOperationException(
                    "Unexpected end of BER data.");
            }

            var tag =
                _buffer[_offset++];

            var length =
                ReadLength();

            if (_offset + length
                > _buffer.Length)
            {
                throw new InvalidOperationException(
                    "BER value exceeds input buffer.");
            }

            var content =
                _buffer
                    .AsSpan(
                        _offset,
                        length)
                    .ToArray();

            _offset +=
                length;

            return new Tlv(
                tag,
                content);
        }

        private int ReadLength()
        {
            if (_offset >= _buffer.Length)
            {
                throw new InvalidOperationException(
                    "Missing BER length.");
            }

            var first =
                _buffer[_offset++];

            if ((first & 0x80) == 0)
            {
                return first;
            }

            var count =
                first & 0x7F;

            if (count is < 1 or > 4)
            {
                throw new InvalidOperationException(
                    "Unsupported BER length.");
            }

            if (_offset + count
                > _buffer.Length)
            {
                throw new InvalidOperationException(
                    "BER length exceeds input buffer.");
            }

            var length = 0;

            for (var index = 0;
                 index < count;
                 index++)
            {
                length =
                    (length << 8)
                    | _buffer[_offset++];
            }

            return length;
        }
    }

    private sealed record Tlv(
        byte Tag,
        byte[] Content);
}
