using Dispatcher.Contracts.Configuration;
using Dispatcher.Contracts.Realtime;
using Dispatcher.Core.Devices;
using Dispatcher.Core.Tags;
using Dispatcher.Server.Realtime;
using Dispatcher.Server.Runtime;
using Microsoft.AspNetCore.SignalR;

namespace Dispatcher.Server.Configuration;

public sealed class ConfigurationEditorService
{
    private readonly SqliteConfigurationStore _store;
    private readonly ConfigurationCatalog _catalog;
    private readonly ModbusRuntimeHostedService _runtime;
    private readonly TagService _tagService;
    private readonly DeviceStateService _deviceStateService;
    private readonly IHubContext<RuntimeHub> _hubContext;
    private readonly SemaphoreSlim _mutationLock = new(1, 1);

    public ConfigurationEditorService(
        SqliteConfigurationStore store,
        ConfigurationCatalog catalog,
        ModbusRuntimeHostedService runtime,
        TagService tagService,
        DeviceStateService deviceStateService,
        IHubContext<RuntimeHub> hubContext)
    {
        _store = store;
        _catalog = catalog;
        _runtime = runtime;
        _tagService = tagService;
        _deviceStateService = deviceStateService;
        _hubContext = hubContext;
    }

    public IReadOnlyList<ModbusDeviceConfiguration> GetDevices()
    {
        return _catalog.Devices;
    }

    public async Task<ModbusDeviceConfiguration> CreateDeviceAsync(
        ModbusDeviceUpsertRequest request,
        CancellationToken cancellationToken)
    {
        return await MutateAsync(
            devices =>
            {
                if (devices.Any(device =>
                        string.Equals(
                            device.DeviceId,
                            request.DeviceId,
                            StringComparison.Ordinal)))
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
        return await MutateAsync(
            devices =>
            {
                var index =
                    FindDeviceIndex(
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
                    && devices.Any(device =>
                        string.Equals(
                            device.DeviceId,
                            request.DeviceId,
                            StringComparison.Ordinal)))
                {
                    throw new ConfigurationConflictException(
                        $"Устройство '{request.DeviceId}' уже существует.");
                }

                var current = devices[index];
                var updated =
                    ConfigurationContractMapper.ToConfiguration(
                        request,
                        current.Tags);

                devices[index] = updated;

                return updated;
            },
            cancellationToken);
    }

    public async Task DeleteDeviceAsync(
        string deviceId,
        CancellationToken cancellationToken)
    {
        await MutateAsync(
            devices =>
            {
                var index =
                    FindDeviceIndex(
                        devices,
                        deviceId);

                if (index < 0)
                {
                    throw new ConfigurationNotFoundException(
                        $"Устройство '{deviceId}' не найдено.");
                }

                devices.RemoveAt(index);

                return true;
            },
            cancellationToken);
    }

    public async Task<ModbusTagConfiguration> CreateTagAsync(
        string deviceId,
        ModbusTagUpsertRequest request,
        CancellationToken cancellationToken)
    {
        return await MutateAsync(
            devices =>
            {
                var index =
                    FindDeviceIndex(
                        devices,
                        deviceId);

                if (index < 0)
                {
                    throw new ConfigurationNotFoundException(
                        $"Устройство '{deviceId}' не найдено.");
                }

                if (ContainsTagId(
                        devices,
                        request.TagId))
                {
                    throw new ConfigurationConflictException(
                        $"Тег '{request.TagId}' уже существует.");
                }

                var created =
                    ConfigurationContractMapper.ToConfiguration(
                        request);

                var device = devices[index];

                devices[index] = device with
                {
                    Tags = device.Tags
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
        return await MutateAsync(
            devices =>
            {
                var deviceIndex =
                    FindDeviceIndex(
                        devices,
                        deviceId);

                if (deviceIndex < 0)
                {
                    throw new ConfigurationNotFoundException(
                        $"Устройство '{deviceId}' не найдено.");
                }

                var device = devices[deviceIndex];
                var tags = device.Tags.ToList();

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
                    && ContainsTagId(
                        devices,
                        request.TagId))
                {
                    throw new ConfigurationConflictException(
                        $"Тег '{request.TagId}' уже существует.");
                }

                var updated =
                    ConfigurationContractMapper.ToConfiguration(
                        request);

                tags[tagIndex] = updated;

                devices[deviceIndex] = device with
                {
                    Tags = tags.ToArray()
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
        await MutateAsync(
            devices =>
            {
                var deviceIndex =
                    FindDeviceIndex(
                        devices,
                        deviceId);

                if (deviceIndex < 0)
                {
                    throw new ConfigurationNotFoundException(
                        $"Устройство '{deviceId}' не найдено.");
                }

                var device = devices[deviceIndex];
                var tags = device.Tags.ToList();

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

                devices[deviceIndex] = device with
                {
                    Tags = tags.ToArray()
                };

                return true;
            },
            cancellationToken);
    }

    private async Task<TResult> MutateAsync<TResult>(
        Func<List<ModbusDeviceConfiguration>, TResult> mutation,
        CancellationToken cancellationToken)
    {
        await _mutationLock.WaitAsync(
            cancellationToken);

        try
        {
            var devices =
                _catalog.Devices
                    .Select(device => device with
                    {
                        Tags = device.Tags.ToArray()
                    })
                    .ToList();

            var result =
                mutation(devices);

            ModbusConfigurationValidator.Validate(
                devices);

            await _store.ReplaceAsync(
                devices,
                cancellationToken);

            _catalog.Replace(
                devices);

            await _runtime.ApplyAsync(
                _catalog.Devices,
                CancellationToken.None);

            await _hubContext.Clients.All.SendAsync(
                RuntimeHubContract.ConfigurationChanged,
                cancellationToken: CancellationToken.None);

            return result;
        }
        finally
        {
            _mutationLock.Release();
        }
    }

    private static int FindDeviceIndex(
        IReadOnlyList<ModbusDeviceConfiguration> devices,
        string deviceId)
    {
        for (var index = 0; index < devices.Count; index++)
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

    private static bool ContainsTagId(
        IEnumerable<ModbusDeviceConfiguration> devices,
        string tagId)
    {
        return devices.Any(device =>
            device.Tags.Any(tag =>
                string.Equals(
                    tag.TagId,
                    tagId,
                    StringComparison.Ordinal)));
    }
}
