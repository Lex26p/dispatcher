using Dispatcher.Contracts.Configuration;
using Dispatcher.Contracts.Realtime;
using Dispatcher.Server.Realtime;
using Dispatcher.Server.Runtime;
using Microsoft.AspNetCore.SignalR;

namespace Dispatcher.Server.Configuration;

public sealed class ConfigurationEditorService
{
    private readonly SqliteConfigurationStore _store;
    private readonly ConfigurationCatalog _catalog;
    private readonly RuntimeConfigurationCoordinator _runtime;
    private readonly IHubContext<RuntimeHub> _hubContext;
    private readonly SemaphoreSlim _mutationLock = new(1, 1);

    public ConfigurationEditorService(
        SqliteConfigurationStore store,
        ConfigurationCatalog catalog,
        RuntimeConfigurationCoordinator runtime,
        IHubContext<RuntimeHub> hubContext)
    {
        _store = store;
        _catalog = catalog;
        _runtime = runtime;
        _hubContext = hubContext;
    }

    public IReadOnlyList<ModbusDeviceConfiguration> GetDevices()
    {
        return _catalog.ModbusDevices;
    }

    public IReadOnlyList<SnmpDeviceConfiguration> GetSnmpDevices()
    {
        return _catalog.SnmpDevices;
    }

    public async Task<ModbusDeviceConfiguration> CreateDeviceAsync(
        ModbusDeviceUpsertRequest request,
        CancellationToken cancellationToken)
    {
        return await MutateModbusAsync(
            devices =>
            {
                if (_catalog.ContainsDeviceId(request.DeviceId))
                {
                    throw new ConfigurationConflictException(
                        $"Устройство '{request.DeviceId}' уже существует.");
                }

                var created =
                    ConfigurationContractMapper.ToConfiguration(
                        request);

                devices.Add(created);

                return created;
            },
            cancellationToken);
    }

    public async Task<ModbusDeviceConfiguration> UpdateDeviceAsync(
        string deviceId,
        ModbusDeviceUpsertRequest request,
        CancellationToken cancellationToken)
    {
        return await MutateModbusAsync(
            devices =>
            {
                var index =
                    FindModbusDeviceIndex(
                        devices,
                        deviceId);

                if (index < 0)
                {
                    throw new ConfigurationNotFoundException(
                        $"Устройство '{deviceId}' не найдено.");
                }

                if (!string.Equals(
                        deviceId,
                        request.DeviceId,
                        StringComparison.Ordinal)
                    && _catalog.ContainsDeviceId(
                        request.DeviceId))
                {
                    throw new ConfigurationConflictException(
                        $"Устройство '{request.DeviceId}' уже существует.");
                }

                var current =
                    devices[index];

                var updated =
                    ConfigurationContractMapper.ToConfiguration(
                        request,
                        current.Tags);

                devices[index] =
                    updated;

                return updated;
            },
            cancellationToken);
    }

    public async Task DeleteDeviceAsync(
        string deviceId,
        CancellationToken cancellationToken)
    {
        await MutateModbusAsync(
            devices =>
            {
                var index =
                    FindModbusDeviceIndex(
                        devices,
                        deviceId);

                if (index < 0)
                {
                    throw new ConfigurationNotFoundException(
                        $"Устройство '{deviceId}' не найдено.");
                }

                devices.RemoveAt(
                    index);

                return true;
            },
            cancellationToken);
    }

    public async Task<ModbusTagConfiguration> CreateTagAsync(
        string deviceId,
        ModbusTagUpsertRequest request,
        CancellationToken cancellationToken)
    {
        return await MutateModbusAsync(
            devices =>
            {
                var index =
                    FindModbusDeviceIndex(
                        devices,
                        deviceId);

                if (index < 0)
                {
                    throw new ConfigurationNotFoundException(
                        $"Устройство '{deviceId}' не найдено.");
                }

                if (_catalog.ContainsTagId(
                        request.TagId))
                {
                    throw new ConfigurationConflictException(
                        $"Тег '{request.TagId}' уже существует.");
                }

                var created =
                    ConfigurationContractMapper.ToConfiguration(
                        request);

                var device =
                    devices[index];

                devices[index] =
                    device with
                    {
                        Tags =
                            device.Tags
                                .Append(created)
                                .ToArray()
                    };

                return created;
            },
            cancellationToken);
    }

    public async Task<ModbusTagConfiguration> UpdateTagAsync(
        string deviceId,
        string tagId,
        ModbusTagUpsertRequest request,
        CancellationToken cancellationToken)
    {
        return await MutateModbusAsync(
            devices =>
            {
                var deviceIndex =
                    FindModbusDeviceIndex(
                        devices,
                        deviceId);

                if (deviceIndex < 0)
                {
                    throw new ConfigurationNotFoundException(
                        $"Устройство '{deviceId}' не найдено.");
                }

                var device =
                    devices[deviceIndex];
                var tags =
                    device.Tags.ToList();

                var tagIndex =
                    tags.FindIndex(tag =>
                        string.Equals(
                            tag.TagId,
                            tagId,
                            StringComparison.Ordinal));

                if (tagIndex < 0)
                {
                    throw new ConfigurationNotFoundException(
                        $"Тег '{tagId}' не найден у устройства '{deviceId}'.");
                }

                if (!string.Equals(
                        tagId,
                        request.TagId,
                        StringComparison.Ordinal)
                    && _catalog.ContainsTagId(
                        request.TagId))
                {
                    throw new ConfigurationConflictException(
                        $"Тег '{request.TagId}' уже существует.");
                }

                var updated =
                    ConfigurationContractMapper.ToConfiguration(
                        request);

                tags[tagIndex] =
                    updated;

                devices[deviceIndex] =
                    device with
                    {
                        Tags =
                            tags.ToArray()
                    };

                return updated;
            },
            cancellationToken);
    }

    public async Task DeleteTagAsync(
        string deviceId,
        string tagId,
        CancellationToken cancellationToken)
    {
        await MutateModbusAsync(
            devices =>
            {
                var deviceIndex =
                    FindModbusDeviceIndex(
                        devices,
                        deviceId);

                if (deviceIndex < 0)
                {
                    throw new ConfigurationNotFoundException(
                        $"Устройство '{deviceId}' не найдено.");
                }

                var device =
                    devices[deviceIndex];
                var tags =
                    device.Tags.ToList();

                var removed =
                    tags.RemoveAll(tag =>
                        string.Equals(
                            tag.TagId,
                            tagId,
                            StringComparison.Ordinal));

                if (removed == 0)
                {
                    throw new ConfigurationNotFoundException(
                        $"Тег '{tagId}' не найден у устройства '{deviceId}'.");
                }

                devices[deviceIndex] =
                    device with
                    {
                        Tags =
                            tags.ToArray()
                    };

                return true;
            },
            cancellationToken);
    }

    public async Task<SnmpDeviceConfiguration> CreateSnmpDeviceAsync(
        SnmpDeviceUpsertRequest request,
        CancellationToken cancellationToken)
    {
        return await MutateSnmpAsync(
            devices =>
            {
                if (_catalog.ContainsDeviceId(request.DeviceId))
                {
                    throw new ConfigurationConflictException(
                        $"Устройство '{request.DeviceId}' уже существует.");
                }

                var created =
                    ConfigurationContractMapper.ToConfiguration(
                        request);

                devices.Add(created);

                return created;
            },
            cancellationToken);
    }

    public async Task<SnmpDeviceConfiguration> UpdateSnmpDeviceAsync(
        string deviceId,
        SnmpDeviceUpsertRequest request,
        CancellationToken cancellationToken)
    {
        return await MutateSnmpAsync(
            devices =>
            {
                var index =
                    FindSnmpDeviceIndex(
                        devices,
                        deviceId);

                if (index < 0)
                {
                    throw new ConfigurationNotFoundException(
                        $"SNMP-устройство '{deviceId}' не найдено.");
                }

                if (!string.Equals(
                        deviceId,
                        request.DeviceId,
                        StringComparison.Ordinal)
                    && _catalog.ContainsDeviceId(
                        request.DeviceId))
                {
                    throw new ConfigurationConflictException(
                        $"Устройство '{request.DeviceId}' уже существует.");
                }

                var current =
                    devices[index];

                var updated =
                    ConfigurationContractMapper.ToConfiguration(
                        request,
                        current.Tags);

                devices[index] =
                    updated;

                return updated;
            },
            cancellationToken);
    }

    public async Task DeleteSnmpDeviceAsync(
        string deviceId,
        CancellationToken cancellationToken)
    {
        await MutateSnmpAsync(
            devices =>
            {
                var index =
                    FindSnmpDeviceIndex(
                        devices,
                        deviceId);

                if (index < 0)
                {
                    throw new ConfigurationNotFoundException(
                        $"SNMP-устройство '{deviceId}' не найдено.");
                }

                devices.RemoveAt(
                    index);

                return true;
            },
            cancellationToken);
    }

    public async Task<SnmpTagConfiguration> CreateSnmpTagAsync(
        string deviceId,
        SnmpTagUpsertRequest request,
        CancellationToken cancellationToken)
    {
        return await MutateSnmpAsync(
            devices =>
            {
                var index =
                    FindSnmpDeviceIndex(
                        devices,
                        deviceId);

                if (index < 0)
                {
                    throw new ConfigurationNotFoundException(
                        $"SNMP-устройство '{deviceId}' не найдено.");
                }

                if (_catalog.ContainsTagId(
                        request.TagId))
                {
                    throw new ConfigurationConflictException(
                        $"Тег '{request.TagId}' уже существует.");
                }

                var created =
                    ConfigurationContractMapper.ToConfiguration(
                        request);

                var device =
                    devices[index];

                devices[index] =
                    device with
                    {
                        Tags =
                            device.Tags
                                .Append(created)
                                .ToArray()
                    };

                return created;
            },
            cancellationToken);
    }

    public async Task<SnmpTagConfiguration> UpdateSnmpTagAsync(
        string deviceId,
        string tagId,
        SnmpTagUpsertRequest request,
        CancellationToken cancellationToken)
    {
        return await MutateSnmpAsync(
            devices =>
            {
                var deviceIndex =
                    FindSnmpDeviceIndex(
                        devices,
                        deviceId);

                if (deviceIndex < 0)
                {
                    throw new ConfigurationNotFoundException(
                        $"SNMP-устройство '{deviceId}' не найдено.");
                }

                var device =
                    devices[deviceIndex];
                var tags =
                    device.Tags.ToList();

                var tagIndex =
                    tags.FindIndex(tag =>
                        string.Equals(
                            tag.TagId,
                            tagId,
                            StringComparison.Ordinal));

                if (tagIndex < 0)
                {
                    throw new ConfigurationNotFoundException(
                        $"Тег '{tagId}' не найден у SNMP-устройства '{deviceId}'.");
                }

                if (!string.Equals(
                        tagId,
                        request.TagId,
                        StringComparison.Ordinal)
                    && _catalog.ContainsTagId(
                        request.TagId))
                {
                    throw new ConfigurationConflictException(
                        $"Тег '{request.TagId}' уже существует.");
                }

                var updated =
                    ConfigurationContractMapper.ToConfiguration(
                        request);

                tags[tagIndex] =
                    updated;

                devices[deviceIndex] =
                    device with
                    {
                        Tags =
                            tags.ToArray()
                    };

                return updated;
            },
            cancellationToken);
    }

    public async Task DeleteSnmpTagAsync(
        string deviceId,
        string tagId,
        CancellationToken cancellationToken)
    {
        await MutateSnmpAsync(
            devices =>
            {
                var deviceIndex =
                    FindSnmpDeviceIndex(
                        devices,
                        deviceId);

                if (deviceIndex < 0)
                {
                    throw new ConfigurationNotFoundException(
                        $"SNMP-устройство '{deviceId}' не найдено.");
                }

                var device =
                    devices[deviceIndex];
                var tags =
                    device.Tags.ToList();

                var removed =
                    tags.RemoveAll(tag =>
                        string.Equals(
                            tag.TagId,
                            tagId,
                            StringComparison.Ordinal));

                if (removed == 0)
                {
                    throw new ConfigurationNotFoundException(
                        $"Тег '{tagId}' не найден у SNMP-устройства '{deviceId}'.");
                }

                devices[deviceIndex] =
                    device with
                    {
                        Tags =
                            tags.ToArray()
                    };

                return true;
            },
            cancellationToken);
    }

    private async Task<TResult> MutateModbusAsync<TResult>(
        Func<List<ModbusDeviceConfiguration>, TResult> mutation,
        CancellationToken cancellationToken)
    {
        await _mutationLock.WaitAsync(
            cancellationToken);

        try
        {
            var devices =
                _catalog.ModbusDevices
                    .Select(device =>
                        device with
                        {
                            Tags =
                                device.Tags.ToArray()
                        })
                    .ToList();

            var result =
                mutation(devices);

            ConfigurationSetValidator.Validate(
                devices,
                _catalog.SnmpDevices);

            await _store.ReplaceAsync(
                devices,
                cancellationToken);

            _catalog.ReplaceModbus(
                devices);

            await PublishConfigurationAsync();

            return result;
        }
        finally
        {
            _mutationLock.Release();
        }
    }

    private async Task<TResult> MutateSnmpAsync<TResult>(
        Func<List<SnmpDeviceConfiguration>, TResult> mutation,
        CancellationToken cancellationToken)
    {
        await _mutationLock.WaitAsync(
            cancellationToken);

        try
        {
            var devices =
                _catalog.SnmpDevices
                    .Select(device =>
                        device with
                        {
                            Tags =
                                device.Tags.ToArray()
                        })
                    .ToList();

            var result =
                mutation(devices);

            ConfigurationSetValidator.Validate(
                _catalog.ModbusDevices,
                devices);

            await _store.ReplaceSnmpAsync(
                devices,
                cancellationToken);

            _catalog.ReplaceSnmp(
                devices);

            await PublishConfigurationAsync();

            return result;
        }
        finally
        {
            _mutationLock.Release();
        }
    }

    private async Task PublishConfigurationAsync()
    {
        await _runtime.ApplyAsync(
            CancellationToken.None);

        await _hubContext.Clients.All.SendAsync(
            RuntimeHubContract.ConfigurationChanged,
            cancellationToken:
                CancellationToken.None);
    }

    private static int FindModbusDeviceIndex(
        IReadOnlyList<ModbusDeviceConfiguration> devices,
        string deviceId)
    {
        for (var index = 0;
             index < devices.Count;
             index++)
        {
            if (string.Equals(
                    devices[index].DeviceId,
                    deviceId,
                    StringComparison.Ordinal))
            {
                return index;
            }
        }

        return -1;
    }

    private static int FindSnmpDeviceIndex(
        IReadOnlyList<SnmpDeviceConfiguration> devices,
        string deviceId)
    {
        for (var index = 0;
             index < devices.Count;
             index++)
        {
            if (string.Equals(
                    devices[index].DeviceId,
                    deviceId,
                    StringComparison.Ordinal))
            {
                return index;
            }
        }

        return -1;
    }
}
