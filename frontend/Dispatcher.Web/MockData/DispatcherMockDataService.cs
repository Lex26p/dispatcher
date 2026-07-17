namespace Dispatcher.Web.MockData;

public sealed class DispatcherMockDataService
{
    private readonly IReadOnlyList<MockObject> _objects =
    [
        new(
            Id: "power",
            Name: "Электроснабжение",
            Group: "Завод №1",
            Type: "Инженерная система",
            Status: "Авария",
            ActiveAlarmCount: 2,
            WidgetCount: 8,
            LastEvent: "3 мин назад",
            IsSystemObject: false
        ),
        new(
            Id: "ventilation",
            Name: "Вентиляция",
            Group: "Завод №1",
            Type: "Инженерная система",
            Status: "Предупреждение",
            ActiveAlarmCount: 1,
            WidgetCount: 12,
            LastEvent: "8 мин назад",
            IsSystemObject: false
        ),
        new(
            Id: "water",
            Name: "Водоснабжение",
            Group: "Завод №1",
            Type: "Инженерная система",
            Status: "Норма",
            ActiveAlarmCount: 0,
            WidgetCount: 6,
            LastEvent: "25 мин назад",
            IsSystemObject: false
        ),
        new(
            Id: "fire",
            Name: "Пожарная система",
            Group: "Завод №1",
            Type: "Безопасность",
            Status: "Норма",
            ActiveAlarmCount: 0,
            WidgetCount: 5,
            LastEvent: "1 час назад",
            IsSystemObject: false
        ),
        new(
            Id: "system-admin",
            Name: "Админка",
            Group: "Система",
            Type: "Системный объект",
            Status: "Норма",
            ActiveAlarmCount: 0,
            WidgetCount: 9,
            LastEvent: "2 часа назад",
            IsSystemObject: true
        ),
        new(
            Id: "system-security",
            Name: "Безопасность",
            Group: "Система",
            Type: "Системный объект",
            Status: "Норма",
            ActiveAlarmCount: 0,
            WidgetCount: 4,
            LastEvent: "2 часа назад",
            IsSystemObject: true
        ),
        new(
            Id: "system-runtime",
            Name: "Runtime",
            Group: "Система",
            Type: "Системный объект",
            Status: "Норма",
            ActiveAlarmCount: 0,
            WidgetCount: 7,
            LastEvent: "5 мин назад",
            IsSystemObject: true
        )
    ];

    private readonly IReadOnlyList<MockAlarm> _alarms =
    [
        new(
            Id: "alarm-001",
            Time: "12:04",
            Level: "Критическая",
            ObjectId: "power",
            ObjectName: "Электроснабжение",
            DeviceName: "Ввод 1",
            TagName: "Напряжение",
            Message: "Ввод 1: потеря напряжения",
            State: "Активна",
            Acknowledged: false,
            Shelved: false,
            Suppressed: false
        ),
        new(
            Id: "alarm-002",
            Time: "12:11",
            Level: "Авария",
            ObjectId: "ventilation",
            ObjectName: "Вентиляция",
            DeviceName: "Вентмашина В-12",
            TagName: "Температура двигателя",
            Message: "Вентмашина В-12: температура двигателя выше 80 °C",
            State: "Активна",
            Acknowledged: false,
            Shelved: false,
            Suppressed: false
        ),
        new(
            Id: "alarm-003",
            Time: "12:18",
            Level: "Предупреждение",
            ObjectId: "ventilation",
            ObjectName: "Вентиляция",
            DeviceName: "Приточная установка П-3",
            TagName: "Перепад давления фильтра",
            Message: "Приточная установка П-3: фильтр загрязнен",
            State: "Активна",
            Acknowledged: true,
            Shelved: false,
            Suppressed: false
        ),
        new(
            Id: "alarm-004",
            Time: "11:40",
            Level: "Системная",
            ObjectId: "system-runtime",
            ObjectName: "Runtime",
            DeviceName: "Runtime",
            TagName: "History pipeline",
            Message: "Runtime: задержка записи истории больше нормы",
            State: "Подтверждена",
            Acknowledged: true,
            Shelved: false,
            Suppressed: false
        )
    ];

    private readonly IReadOnlyList<MockEvent> _events =
    [
        new(
            Id: "event-001",
            Time: "12:04",
            Type: "Авария",
            Level: "Критическая",
            ObjectName: "Электроснабжение",
            UserName: null,
            Message: "Появилась авария: Ввод 1: потеря напряжения"
        ),
        new(
            Id: "event-002",
            Time: "12:05",
            Type: "Действие оператора",
            Level: "Информация",
            ObjectName: "Электроснабжение",
            UserName: "Оператор",
            Message: "Оператор открыл объект Электроснабжение"
        ),
        new(
            Id: "event-003",
            Time: "12:11",
            Type: "Авария",
            Level: "Авария",
            ObjectName: "Вентиляция",
            UserName: null,
            Message: "Появилась авария: температура двигателя выше 80 °C"
        ),
        new(
            Id: "event-004",
            Time: "12:18",
            Type: "Авария",
            Level: "Предупреждение",
            ObjectName: "Вентиляция",
            UserName: null,
            Message: "Появилось предупреждение: фильтр загрязнен"
        ),
        new(
            Id: "event-005",
            Time: "12:20",
            Type: "Подтверждение аварии",
            Level: "Информация",
            ObjectName: "Вентиляция",
            UserName: "Оператор",
            Message: "Авария по фильтру подтверждена"
        )
    ];

    private readonly IReadOnlyList<MockDevice> _devices =
    [
        new(
            Id: "dev-power-meter-1",
            Name: "Счетчик ввода 1",
            Type: "Электросчетчик",
            ObjectId: "power",
            ObjectName: "Электроснабжение",
            Location: "ГРЩ, шкаф 1",
            Protocol: "Modbus TCP",
            Endpoint: "192.168.10.21:502",
            Status: "На связи",
            TagCount: 16,
            LastPoll: "5 сек назад"
        ),
        new(
            Id: "dev-ahu-12",
            Name: "Вентмашина В-12",
            Type: "Вентиляционная установка",
            ObjectId: "ventilation",
            ObjectName: "Вентиляция",
            Location: "Кровля, зона 2",
            Protocol: "Modbus TCP",
            Endpoint: "192.168.10.42:502",
            Status: "Авария",
            TagCount: 24,
            LastPoll: "8 сек назад"
        ),
        new(
            Id: "dev-water-meter-1",
            Name: "Счетчик воды 1",
            Type: "Счетчик воды",
            ObjectId: "water",
            ObjectName: "Водоснабжение",
            Location: "Насосная",
            Protocol: "Modbus RTU",
            Endpoint: "COM3 / 9600 / slave 4",
            Status: "На связи",
            TagCount: 8,
            LastPoll: "10 сек назад"
        )
    ];

    private readonly IReadOnlyList<MockTag> _tags =
    [
        new(
            Id: "tag-voltage-l1",
            Name: "power.input1.voltage_l1",
            DisplayName: "Напряжение L1",
            DeviceId: "dev-power-meter-1",
            DeviceName: "Счетчик ввода 1",
            ObjectName: "Электроснабжение",
            Unit: "В",
            Value: "0",
            Quality: "Хорошее",
            SignalType: "Analog"
        ),
        new(
            Id: "tag-ahu12-motor-temp",
            Name: "vent.ahu12.motor_temp",
            DisplayName: "Температура двигателя",
            DeviceId: "dev-ahu-12",
            DeviceName: "Вентмашина В-12",
            ObjectName: "Вентиляция",
            Unit: "°C",
            Value: "84.2",
            Quality: "Хорошее",
            SignalType: "Analog"
        ),
        new(
            Id: "tag-ahu12-run",
            Name: "vent.ahu12.run",
            DisplayName: "Работа",
            DeviceId: "dev-ahu-12",
            DeviceName: "Вентмашина В-12",
            ObjectName: "Вентиляция",
            Unit: "",
            Value: "Включена",
            Quality: "Хорошее",
            SignalType: "Discrete"
        ),
        new(
            Id: "tag-water-flow",
            Name: "water.meter1.flow",
            DisplayName: "Расход воды",
            DeviceId: "dev-water-meter-1",
            DeviceName: "Счетчик воды 1",
            ObjectName: "Водоснабжение",
            Unit: "м³/ч",
            Value: "12.8",
            Quality: "Хорошее",
            SignalType: "Analog"
        )
    ];

    public IReadOnlyList<MockObject> GetObjects()
    {
        return _objects;
    }

    public IReadOnlyList<MockAlarm> GetAlarms()
    {
        return _alarms;
    }

    public IReadOnlyList<MockAlarm> GetActiveAlarms()
    {
        return _alarms
            .Where(alarm => alarm.State is "Активна")
            .ToList();
    }

    public IReadOnlyList<MockEvent> GetEvents()
    {
        return _events;
    }

    public IReadOnlyList<MockDevice> GetDevices()
    {
        return _devices;
    }

    public IReadOnlyList<MockTag> GetTags()
    {
        return _tags;
    }

    public MockObject? GetObjectById(string id)
    {
        return _objects.FirstOrDefault(item => item.Id == id);
    }

    public IReadOnlyList<MockAlarm> GetAlarmsByObjectId(string objectId)
    {
        return _alarms
            .Where(alarm => alarm.ObjectId == objectId)
            .ToList();
    }

    public IReadOnlyList<MockTag> GetTagsByDeviceId(string deviceId)
    {
        return _tags
            .Where(tag => tag.DeviceId == deviceId)
            .ToList();
    }
}