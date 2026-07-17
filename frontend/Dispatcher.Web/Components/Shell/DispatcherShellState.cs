namespace Dispatcher.Web.Components.Shell;

public sealed class DispatcherShellState
{
    private string _pageTitle = "Обзор системы";
    private int _activeAlarmCount;
    private bool _controlModeAvailable;
    private bool _controlModeEnabled;
    private TimeSpan? _controlModeRemaining;
    private bool _alarmPanelOpen;
    private bool _alarmPanelExpanded;

    public event Action? Changed;

    public string PageTitle => _pageTitle;

    public int ActiveAlarmCount => _activeAlarmCount;

    public bool HasActiveAlarms => _activeAlarmCount > 0;

    public bool ControlModeAvailable => _controlModeAvailable;

    public bool ControlModeEnabled => _controlModeEnabled;

    public TimeSpan? ControlModeRemaining => _controlModeRemaining;

    public bool AlarmPanelOpen => _alarmPanelOpen;

    public bool AlarmPanelExpanded => _alarmPanelExpanded;

    public void SetPageTitle(string title)
    {
        var normalized =
            string.IsNullOrWhiteSpace(title)
                ? "Диспетчер"
                : title.Trim();

        if (_pageTitle == normalized)
        {
            return;
        }

        _pageTitle = normalized;
        NotifyChanged();
    }

    public void SetActiveAlarmCount(int count)
    {
        var normalized = Math.Max(0, count);

        if (_activeAlarmCount == normalized)
        {
            return;
        }

        _activeAlarmCount = normalized;
        NotifyChanged();
    }

    public void SetControlModeAvailable(bool available)
    {
        if (_controlModeAvailable == available)
        {
            return;
        }

        _controlModeAvailable = available;

        if (!available)
        {
            _controlModeEnabled = false;
            _controlModeRemaining = null;
        }

        NotifyChanged();
    }

    public void SetControlMode(bool enabled, TimeSpan? remaining = null)
    {
        if (_controlModeEnabled == enabled && _controlModeRemaining == remaining)
        {
            return;
        }

        _controlModeEnabled = enabled;
        _controlModeRemaining = enabled ? remaining : null;
        NotifyChanged();
    }

    public void OpenAlarmPanel()
    {
        if (_alarmPanelOpen)
        {
            return;
        }

        _alarmPanelOpen = true;
        NotifyChanged();
    }

    public void CloseAlarmPanel()
    {
        if (!_alarmPanelOpen)
        {
            return;
        }

        _alarmPanelOpen = false;
        NotifyChanged();
    }

    public void ToggleAlarmPanel()
    {
        _alarmPanelOpen = !_alarmPanelOpen;
        NotifyChanged();
    }

    public void ToggleAlarmPanelExpanded()
    {
        _alarmPanelExpanded = !_alarmPanelExpanded;
        NotifyChanged();
    }

    private void NotifyChanged()
    {
        Changed?.Invoke();
    }
}